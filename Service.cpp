#include "Service.h"


// static variable initialization with default hardcoded values
int Service::g_fetchingIntervalSeconds = DEFAULT_FETCHING_INTERVAL_SECONDS;
std::string Service::g_currencyCodes = DEFAULT_CURRENCY_CODES;
std::string Service::g_logFileName = DEFAULT_LOG_FILENAME;
std::string Service::g_dataFileName = DEFAULT_DATA_FILENAME;

// Callback function that gets triggered when registry key registered in CSampleService::monitorRegistry() changed
// updates static config variables by reading from service registry
VOID CALLBACK RegistryChangeCallback(PVOID hEvent, BOOLEAN fTimeout)
{
    log(Service::getLogFileName(), get_current_time() + " RegistryChangeCallback(): received registry change callback.\r\n");
    std::string REGISTRY_KEY = REGISTRY_KEY_DIR + PWSTRToStdString(Service::GetName());

    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, REGISTRY_KEY.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        bool regQueryFailed{ false };

        // Read FetchInterval int value
        DWORD fetchingIntervalSeconds = 0;
        DWORD dwSizeFetchInterval = sizeof(fetchingIntervalSeconds);
        if (ERROR_SUCCESS == RegQueryValueExA(hKey, REGISTRY_VALUE_FETCHINTERVAL, nullptr, nullptr, reinterpret_cast<LPBYTE>(&fetchingIntervalSeconds), &dwSizeFetchInterval))
        {
            Service::setFetchingIntervalSeconds(static_cast<int>(fetchingIntervalSeconds));
        }
        else
        {
            regQueryFailed = true;
        }
        //read all other string values
        const std::vector<std::pair<const char*, std::function<void(const std::string&)>>> registryEntries = {
            {REGISTRY_VALUE_LOGFILE, [](const std::string& value) { Service::setLogFileName(value); }},
            {REGISTRY_VALUE_DATAFILE, [](const std::string& value) { Service::setDataFileName(value); }},
            {REGISTRY_VALUE_CURRENCY_CODES, [](const std::string& value) { Service::setCurrencyCodes(value); }}
        };

        for (const auto& entry : registryEntries)
        {
            char valueStr[MAX_PATH];
            DWORD dwSize = sizeof(valueStr);
            if (ERROR_SUCCESS == RegQueryValueExA(hKey, entry.first, nullptr, nullptr, reinterpret_cast<LPBYTE>(valueStr), &dwSize))
            {
                entry.second(valueStr);
            }
            else
            {
                regQueryFailed = true;
            }
        }

        if (regQueryFailed)
        {
            log(Service::getLogFileName(), "RegistryChangeCallback(): Failed to retrieve data from registry.\r\n");
        }

        RegCloseKey(hKey);
    }
    else
    {
        log(Service::getLogFileName(), "RegistryChangeCallback(): Failed to open registry key.\r\n");
    }
}


int Service::monitorRegistry()
{
    // Open the registry key for monitoring
    std::string REGISTRY_KEY = REGISTRY_KEY_DIR + PWSTRToStdString(Service::GetName());
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, REGISTRY_KEY.c_str(), 0, KEY_NOTIFY, &hKey) != ERROR_SUCCESS)
    {
        log(getLogFileName(), "monitorRegistry(): Failed to open registry key for monitoring.\r\n");
        return -1;
    }

    // Create an event object
    HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (hEvent == NULL)
    {
        log(getLogFileName(), "monitorRegistry(): Failed to create event object.\r\n");
        RegCloseKey(hKey);
        return -1;
    }

    // Register the callback function for registry change notification
    if (RegNotifyChangeKeyValue(hKey, TRUE, REG_NOTIFY_CHANGE_LAST_SET, hEvent, TRUE) != ERROR_SUCCESS)
    {
        log(getLogFileName(), "monitorRegistry(): Failed to register callback for registry change notification.\r\n");
        CloseHandle(hEvent);
        RegCloseKey(hKey);
        return -1;
    }

    // Wait for registry changes
    log(getLogFileName(), "Waiting for registry changes...\r\n");
    while (!m_serviceStopping)
    {
        // Wait for the event to be signaled
        if (WaitForSingleObject(hEvent, INFINITE) != WAIT_OBJECT_0)
        {
            log(getLogFileName(), "monitorRegistry(): Failed to wait for registry changes.\r\n");
            break;
        }

        // Call the callback function
        RegistryChangeCallback(nullptr, FALSE);

        // Reset the event to non-signaled state
        if (!ResetEvent(hEvent))
        {
            log(getLogFileName(), "monitorRegistry(): Failed to reset event.\r\n");
            break;
        }

        // Re-register for registry change notifications
        if (RegNotifyChangeKeyValue(hKey, TRUE, REG_NOTIFY_CHANGE_LAST_SET, hEvent, TRUE) != ERROR_SUCCESS)
        {
            log(getLogFileName(), "monitorRegistry(): Failed to re-register callback for registry change notification.\r\n");
            break;
        }
    }

    // Clean up
    CloseHandle(hEvent);
    RegCloseKey(hKey);

    return 0;
}

void Service::fetch()
{
    while (!m_serviceStopping)
    {
        try
        {
            using namespace web;
            using namespace web::http;
            using namespace web::http::client;

            http_client client(NBU_HOSTNAME);
            http_response response = client.request(methods::GET, NBU_QUERY).get();

            if (response.status_code() == status_codes::OK)
            {
                response.extract_string().then([this](utility::string_t responseBody)
                    {
                        json::value jsonResponse = json::value::parse(responseBody);
                        std::string fileName = getDataFileName();
                        for (const auto& currency : jsonResponse.as_array()) {
                            utility::string_t cc = currency.at(L"cc").as_string();
                            //converting currecy code string to currency code vector
                            std::string currencyCodes = getCurrencyCodes();
                            std::vector<std::string> currencyCodesVector;
                             std::stringstream ss(currencyCodes);
                             std::string token;
                             while (std::getline(ss, token, ','))
                             {
                                 token.erase(std::remove(token.begin(), token.end(), ' '), token.end()); // trimming spaces
                                 currencyCodesVector.push_back(token);
                             }
                             //check if we should log it
                            for (std::string code : currencyCodesVector) {
                                if (utility::conversions::to_utf8string(cc) == code) {
                                    double rate = currency.at(U("rate")).as_double();
                                    log(fileName, get_current_time() + "," + utility::conversions::to_utf8string(cc) + "," + std::to_string(rate));
                                }
                            }
                        } })
                    .wait();
            }
            else
            {
                log(getLogFileName(), "fetch(): Failed to get the HTTP response.\r\n");
            }
        }
        catch (const std::exception& ex)
        {
            log(getLogFileName(), "fetch(): Exception during HTTP request: " + std::string(ex.what()) + "\r\n");
        }

        ::Sleep(getFetchingIntervalSeconds() * 1000);
    }

    // Signal the stopped event.
    SetEvent(m_hStoppedEvent);
}

Service::Service(PWSTR pszServiceName,
    BOOL fCanStop,
    BOOL fCanShutdown,
    BOOL fCanPauseContinue)
    : ServiceBase(pszServiceName, fCanStop, fCanShutdown, fCanPauseContinue)
{
    m_serviceStopping = FALSE;

    // Create a manual-reset event that is not signaled at first to indicate
    // the stopped signal of the service.
    m_hStoppedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (m_hStoppedEvent == NULL)
    {
        throw GetLastError();
    }

    log(Service::getLogFileName(), get_current_time() + " Service constructed.\r\n");
}

Service::~Service(void)
{
    if (m_hStoppedEvent)
    {
        CloseHandle(m_hStoppedEvent);
        m_hStoppedEvent = NULL;
    }
    log(Service::getLogFileName(), get_current_time() + " Service deconstructed.\r\n");
}

//
//   FUNCTION: CSampleService::OnStart(DWORD, LPWSTR *)
//
//   PURPOSE: The function is executed when a Start command is sent to the
//   service by the SCM or when the operating system starts (for a service
//   that starts automatically). It specifies actions to take when the
//   service starts. In this code sample, OnStart logs a service-start
//   message to the Application log, and queues the main service function for
//   execution in a thread pool worker thread.
//
//   PARAMETERS:
//   * dwArgc   - number of command line arguments
//   * lpszArgv - array of command line arguments
//
//   NOTE: A service application is designed to be long running. Therefore,
//   it usually polls or monitors something in the system. The monitoring is
//   set up in the OnStart method. However, OnStart does not actually do the
//   monitoring. The OnStart method must return to the operating system after
//   the service's operation has begun. It must not loop forever or block. To
//   set up a simple monitoring mechanism, one general solution is to create
//   a timer in OnStart. The timer would then raise events in your code
//   periodically, at which time your service could do its monitoring. The
//   other solution is to spawn a new thread to perform the main service
//   functions, which is demonstrated in this code sample.
//
void Service::OnStart(DWORD dwArgc, LPWSTR* lpszArgv)
{
    std::wstring message = std::wstring(GetName()) + L" in OnStart";
    PWSTR pwstrMessage = const_cast<PWSTR>(message.c_str());
    // Log a service start message to the Application log.
    WriteEventLogEntry(pwstrMessage,
        EVENTLOG_INFORMATION_TYPE);
    log(Service::getLogFileName(), get_current_time() + " Service started.\r\n");

    std::thread thrFetch(&Service::fetch, this);
    std::thread thrRegMonitor(&Service::monitorRegistry, this);
    thrFetch.detach();
    thrRegMonitor.detach();
}

//
//   FUNCTION: CSampleService::ServiceWorkerThread(void)
//
//   PURPOSE: The method performs the main function of the service. It runs
//   on a thread pool worker thread.
//

//
//   FUNCTION: CSampleService::OnStop(void)
//
//   PURPOSE: The function is executed when a Stop command is sent to the
//   service by SCM. It specifies actions to take when a service stops
//   running. In this code sample, OnStop logs a service-stop message to the
//   Application log, and waits for the finish of the main service function.
//
//   COMMENTS:
//   Be sure to periodically call ReportServiceStatus() with
//   SERVICE_STOP_PENDING if the procedure is going to take long time.
//
void Service::OnStop()
{
    std::wstring message = std::wstring(GetName()) + L" in OnStop";
    PWSTR pwstrMessage = const_cast<PWSTR>(message.c_str());
    // Log a service stop message to the Application log.
    WriteEventLogEntry(pwstrMessage,
        EVENTLOG_INFORMATION_TYPE);
    log(Service::getLogFileName(), get_current_time() + " Service stopped.\r\n");

    // Indicate that the service is stopping and wait for the finish of the
    // main service function (ServiceWorkerThread).
    m_serviceStopping = TRUE;
    if (WaitForSingleObject(m_hStoppedEvent, INFINITE) != WAIT_OBJECT_0)
    {
        throw GetLastError();
    }
}

// getters

int& Service::getFetchingIntervalSeconds()
{
    return g_fetchingIntervalSeconds;
}
std::string& Service::getLogFileName()
{
    return g_logFileName;
}
std::string& Service::getDataFileName()
{
    return g_dataFileName;
}
std::string& Service::getCurrencyCodes()
{
    return g_currencyCodes;
}

// setters

void Service::setFetchingIntervalSeconds(const int& interval)
{
    g_fetchingIntervalSeconds = interval;
}
void Service::setCurrencyCodes(const std::string& currencyCodes)
{
    g_currencyCodes = currencyCodes;
}
void Service::setLogFileName(const std::string& name)
{
    g_logFileName = name;
}
void Service::setDataFileName(const std::string& name)
{
    g_dataFileName = name;
}
