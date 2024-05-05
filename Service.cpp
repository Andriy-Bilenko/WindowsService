#pragma region Includes
#include <iostream>
#include <fstream>
#include <ctime>
#include <thread>
#include "Service.h"
#include <cpprest/http_client.h>
#include <cpprest/filestream.h>
#include <cpprest/uri.h>
#include <cpprest/json.h>
#include "ServiceManager.h"
#include "helperFunctions.h"
#pragma endregion

// static variable initialization with default hardcoded values
int Service::g_fetchingIntervalSeconds = 2;
std::vector<std::string> Service::g_currencyCodes = { "USD" };
std::string Service::g_logFileName = "C:\\dir\\default_logs.txt";
std::string Service::g_dataFileName = "C:\\dir\\default_currency_data.csv";

// Callback function that gets triggered when registry key registered in CSampleService::monitorRegistry() changed
// updates static config variables by reading from service registry
VOID CALLBACK RegistryChangeCallback(PVOID hEvent, BOOLEAN fTimeout)
{
    std::string REGISTRY_KEY = "SYSTEM\\CurrentControlSet\\Services\\" + PWSTRToStdString(Service::GetName());
    const char* REGISTRY_VALUE_FETCHINTERVAL = "FetchInterval";
    const char* REGISTRY_VALUE_LOGFILE = "LogFile";
    const char* REGISTRY_VALUE_DATAFILE = "DataFile";
    const char* REGISTRY_VALUE_CUURENCY_CODES = "CurrencyCodes";

    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, REGISTRY_KEY.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        bool regQueryFailed{ false };
        // Read FetchInterval value
        DWORD fetchingIntervalSeconds = 0;
        DWORD dwSize = sizeof(fetchingIntervalSeconds);
        if (ERROR_SUCCESS == RegQueryValueExA(hKey, REGISTRY_VALUE_FETCHINTERVAL, nullptr, nullptr, reinterpret_cast<LPBYTE>(&fetchingIntervalSeconds), &dwSize))
        {
            int fetchingIntervalSecondsInt = static_cast<int>(fetchingIntervalSeconds);
            Service::setFetchingIntervalSeconds(fetchingIntervalSecondsInt);
        }
        else
        {
            regQueryFailed = true;
        }
        // Read LogFile value
        char logFileNameStr[MAX_PATH];
        std::string logFileName;
        dwSize = sizeof(logFileNameStr);
        if (ERROR_SUCCESS == RegQueryValueExA(hKey, REGISTRY_VALUE_LOGFILE, nullptr, nullptr, reinterpret_cast<LPBYTE>(logFileNameStr), &dwSize))
        {
            logFileName = std::string(logFileNameStr);
            Service::setLogFileName(logFileName);
        }
        else
        {
            regQueryFailed = true;
        }
        // Read DataFile value
        char dataFileNameStr[MAX_PATH];
        std::string dataFileName;
        dwSize = sizeof(dataFileNameStr);
        if (ERROR_SUCCESS == RegQueryValueExA(hKey, REGISTRY_VALUE_DATAFILE, nullptr, nullptr, reinterpret_cast<LPBYTE>(dataFileNameStr), &dwSize))
        {
            dataFileName = std::string(dataFileNameStr);
            Service::setDataFileName(dataFileName);
        }
        else
        {
            regQueryFailed = true;
        }
        // Read CurrencyCodes value
        char currencyCodesStr[MAX_PATH];
        std::string currencyCodes;
        dwSize = sizeof(currencyCodesStr);
        if (ERROR_SUCCESS == RegQueryValueExA(hKey, REGISTRY_VALUE_CUURENCY_CODES, nullptr, nullptr, reinterpret_cast<LPBYTE>(currencyCodesStr), &dwSize))
        {
            currencyCodes = std::string(currencyCodesStr);
            std::vector<std::string> currencyCodesVector;
            std::stringstream ss(currencyCodes);
            std::string token;
            while (std::getline(ss, token, ','))
            {
                token.erase(std::remove(token.begin(), token.end(), ' '), token.end()); // trimming spaces
                currencyCodesVector.push_back(token);
            }
            Service::setCurrencyCodes(currencyCodesVector);
        }
        else
        {
            regQueryFailed = true;
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
    std::string REGISTRY_KEY = "SYSTEM\\CurrentControlSet\\Services\\" + PWSTRToStdString(Service::GetName());
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
        using namespace web;
        using namespace web::http;
        using namespace web::http::client;

        try
        {
            http_client client(U("https://bank.gov.ua"));
            http_response response = client.request(methods::GET, U("/NBUStatService/v1/statdirectory/exchange?json")).get();

            if (response.status_code() == status_codes::OK)
            {
                response.extract_string().then([this](utility::string_t responseBody)
                    {
                        json::value jsonResponse = json::value::parse(responseBody);
                        std::string fileName = getDataFileName();
                        for (const auto& currency : jsonResponse.as_array()) {
                            utility::string_t cc = currency.at(U("cc")).as_string();
                            //check if we should log it
                            std::vector<std::string> currencyCodes = getCurrencyCodes();
                            for (std::string code : currencyCodes) {
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
}

Service::~Service(void)
{
    if (m_hStoppedEvent)
    {
        CloseHandle(m_hStoppedEvent);
        m_hStoppedEvent = NULL;
    }
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
std::vector<std::string>& Service::getCurrencyCodes()
{
    return g_currencyCodes;
}

// setters

void Service::setFetchingIntervalSeconds(int& interval)
{
    g_fetchingIntervalSeconds = interval;
}
void Service::setCurrencyCodes(std::vector<std::string>& currencyCodes)
{
    g_currencyCodes = currencyCodes;
}
void Service::setLogFileName(std::string& name)
{
    g_logFileName = name;
}
void Service::setDataFileName(std::string& name)
{
    g_dataFileName = name;
}
