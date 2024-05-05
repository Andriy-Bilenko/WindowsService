#include "ServiceManager.h"
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include "Service.h"
#include "helperFunctions.h"

int ServiceManager::registerServiceWithSCM(PWSTR pszServiceName)
{
    // register with the Service Control Manager(SCM)
    Service service(pszServiceName);
    if (!ServiceBase::Run(service))
    {
        std::cerr << "RegisterServiceWithSCM: Service failed to run w/err " << GetLastError() << "\r\n";
        std::cerr << "this function should be ran by Service Control Manager and not from command line\r\n";
        return -1;
    }
    return 0;
}

int ServiceManager::installService(PWSTR pszServiceName,
    PWSTR pszDisplayName,
    DWORD dwStartType,
    PWSTR pszDependencies,
    PWSTR pszAccount,
    PWSTR pszPassword)
{
    wchar_t szPath[MAX_PATH];
    SC_HANDLE schSCManager = NULL;
    SC_HANDLE schService = NULL;

    if (GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath)) == 0)
    {
        std::cerr << "InstallService(): GetModuleFileName failed w/err " << GetLastError() << "\r\n";
        return -1;
    }

    // Open the local default service control manager database
    schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE);
    if (schSCManager == NULL)
    {
        std::cerr << "InstallService(): OpenSCManager failed w/err " << GetLastError() << "\r\n";
        return -1;
    }

    // Install the service into SCM by calling CreateService
    schService = CreateService(
        schSCManager,              // SCManager database
        pszServiceName,            // Name of service
        pszDisplayName,            // Name to display
        SERVICE_QUERY_STATUS,      // Desired access
        SERVICE_WIN32_OWN_PROCESS, // Service type
        dwStartType,               // Service start type
        SERVICE_ERROR_NORMAL,      // Error control type
        szPath,                    // Service's binary
        NULL,                      // No load ordering group
        NULL,                      // No tag identifier
        pszDependencies,           // Dependencies
        pszAccount,                // Service running account
        pszPassword                // Password of the account
    );
    if (schService == NULL)
    {
        std::cerr << "InstallService(): CreateService failed w/err " << GetLastError() << "\r\n";
        CloseServiceHandle(schSCManager);
        return -1;
    }

    std::clog << PWSTRToStdString(pszServiceName) << " is installed\r\n";

    CloseServiceHandle(schSCManager);
    CloseServiceHandle(schService);

    // write default configuration to the registry:
    std::string logFileName = "C:\\dir\\default_logs.txt";
    std::string dataFileName = "C:\\dir\\default_currency_data.csv";
    int fetchingIntervalSeconds{ 2 };
    std::string currencyCodes = { "USD" };
    std::string serviceName = PWSTRToStdString(pszServiceName);
    writeToRegistry(fetchingIntervalSeconds, logFileName, dataFileName, currencyCodes, serviceName);

    return 0;
}

int ServiceManager::removeService(PWSTR pszServiceName)
{
    SC_HANDLE schSCManager = NULL;
    SC_HANDLE schService = NULL;
    SERVICE_STATUS ssSvcStatus = {};

    // Open the local default service control manager database
    schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (schSCManager == NULL)
    {
        std::cerr << "RemoveService(): OpenSCManager failed w/err " << GetLastError() << "\r\n";
        return -1;
    }

    // Open the service with delete, stop, and query status permissions
    schService = OpenService(schSCManager, pszServiceName, SERVICE_STOP | SERVICE_QUERY_STATUS | DELETE);
    if (schService == NULL)
    {
        std::cerr << "RemoveService(): OpenService failed w/err " << GetLastError() << "\r\n";
        CloseServiceHandle(schSCManager);
        return -1;
    }

    // Try to stop the service
    if (ControlService(schService, SERVICE_CONTROL_STOP, &ssSvcStatus))
    {
        std::clog << "Stopping " << pszServiceName;
        Sleep(1000);

        while (QueryServiceStatus(schService, &ssSvcStatus))
        {
            if (ssSvcStatus.dwCurrentState == SERVICE_STOP_PENDING)
            {
                std::clog << ".";
                Sleep(1000);
            }
            else
                break;
        }

        if (ssSvcStatus.dwCurrentState == SERVICE_STOPPED)
        {
            std::clog << pszServiceName << " is stopped\r\n";
        }
        else
        {
            std::cerr << pszServiceName << " failed to stop\r\n";
            CloseServiceHandle(schSCManager);
            CloseServiceHandle(schService);
            return -1;
        }
    }

    // Now remove the service by calling DeleteService.
    bool deleteSuccess = DeleteService(schService);
    CloseServiceHandle(schSCManager);
    CloseServiceHandle(schService);
    if (!deleteSuccess)
    {
        std::cerr << "RemoveService(): DeleteService failed w/err " << GetLastError() << "\r\n";
        return -1;
    }

    std::clog << PWSTRToStdString(pszServiceName) << " is removed\r\n";

    return 0;
}

int ServiceManager::startService(PWSTR pszServiceName)
{
    // Open the service control manager database
    SC_HANDLE schSCManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (schSCManager == nullptr)
    {
        std::cerr << "StartService(): Failed to open service control manager database. Error code: " << GetLastError() << "\r\n";
        return -1;
    }

    // Open the service
    SC_HANDLE schService = OpenService(schSCManager, pszServiceName, SERVICE_START);
    if (schService == nullptr)
    {
        std::cerr << "StartService(): Failed to open service. Error code: " << GetLastError() << "\r\n";
        CloseServiceHandle(schSCManager);
        return -1;
    }

    // Start the service
    bool startSuccess = StartService(schService, 0, nullptr);
    // cleanup
    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
    if (!startSuccess)
    {
        std::cerr << "StartService(): Failed to start service. Error code: " << GetLastError() << "\r\n";
        return -1;
    }
    else
    {
        std::clog << "Service started successfully.\r\n";
        return 0;
    }
}

int ServiceManager::stopService(PWSTR pszServiceName)
{
    // Open the service control manager database
    SC_HANDLE schSCManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (schSCManager == nullptr)
    {
        std::cerr << "StopService(): Failed to open service control manager database. Error code: " << GetLastError() << "\r\n";
        return -1;
    }

    // Open the service
    SC_HANDLE schService = OpenService(schSCManager, pszServiceName, SERVICE_STOP);
    if (schService == nullptr)
    {
        std::cerr << "StopService(): Failed to open service. Error code: " << GetLastError() << "\r\n";
        CloseServiceHandle(schSCManager);
        return -1;
    }

    // Send a stop control code to the service
    SERVICE_STATUS status;
    bool stopSuccess = ControlService(schService, SERVICE_CONTROL_STOP, &status);
    // Close service handles
    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
    if (!stopSuccess)
    {
        std::cerr << "StopService(): Failed to stop service. Error code: " << GetLastError() << "\r\n";
        return -1;
    }
    else
    {
        std::cout << "Service stopped successfully.\r\n";
        return 0;
    }
}

int ServiceManager::loadConfigsFromFile(PWSTR pszServiceName, PWSTR iniFileName)
{
    // read ini
    std::string logFileName, dataFileName, currencyCodes;
    int fetchingIntervalSeconds{};
    parseIni(fetchingIntervalSeconds, logFileName, dataFileName, currencyCodes, iniFileName);
    // write to registry
    std::string serviceName = PWSTRToStdString(pszServiceName);
    return writeToRegistry(fetchingIntervalSeconds, logFileName, dataFileName, currencyCodes, serviceName);
}

int ServiceManager::writeToRegistry(const int& fetchingIntervalSeconds, const std::string& logFileName, const std::string& dataFileName, const std::string& currencyCodes, const std::string& serviceName)
{
    std::string REGISTRY_KEY = "SYSTEM\\CurrentControlSet\\Services\\" + serviceName;
    const char* REGISTRY_VALUE_FETCHINTERVAL = "FetchInterval";
    const char* REGISTRY_VALUE_LOGFILE = "LogFile";
    const char* REGISTRY_VALUE_DATAFILE = "DataFile";
    const char* REGISTRY_VALUE_CUURENCY_CODES = "CurrencyCodes";

    HKEY hKey;
    DWORD disposition;
    if (RegCreateKeyExA(HKEY_LOCAL_MACHINE, REGISTRY_KEY.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, &disposition) == ERROR_SUCCESS)
    {
        bool settingValueFailed{ false };
        LONG result;
        result = RegSetValueExA(hKey, REGISTRY_VALUE_FETCHINTERVAL, 0, REG_DWORD, reinterpret_cast<const BYTE*>(&fetchingIntervalSeconds), sizeof(DWORD));
        if (result != ERROR_SUCCESS)
        {
            settingValueFailed = true;
        }
        result = RegSetValueExA(hKey, REGISTRY_VALUE_LOGFILE, 0, REG_SZ, reinterpret_cast<const BYTE*>(logFileName.c_str()), static_cast<DWORD>(logFileName.size() + 1));
        if (result != ERROR_SUCCESS)
        {
            settingValueFailed = true;
        }
        result = RegSetValueExA(hKey, REGISTRY_VALUE_DATAFILE, 0, REG_SZ, reinterpret_cast<const BYTE*>(dataFileName.c_str()), static_cast<DWORD>(dataFileName.size() + 1));
        if (result != ERROR_SUCCESS)
        {
            settingValueFailed = true;
        }
        result = RegSetValueExA(hKey, REGISTRY_VALUE_CUURENCY_CODES, 0, REG_SZ, reinterpret_cast<const BYTE*>(currencyCodes.c_str()), static_cast<DWORD>(currencyCodes.size() + 1));
        if (result != ERROR_SUCCESS)
        {
            settingValueFailed = true;
        }
        if (settingValueFailed)
        {
            std::cerr << "WriteToRegistry(): Setting registry value failed w/err " << GetLastError() << "\r\n";
            return -1;
        }

        RegCloseKey(hKey);
    }
    else
    {
        std::cerr << "WriteToRegistry(): Creating registry key failed w/err " << GetLastError() << "\r\n";
        return -1;
    }
    return 0;
}

void ServiceManager::parseIni(int& fetchingIntervalSeconds, std::string& logFileName, std::string& dataFileName, std::string& currencyCodes, PWSTR iniFileName)
{
    wchar_t logFile[256];
    wchar_t dataFile[256];
    wchar_t wcharCurrencyCodes[256];
    fetchingIntervalSeconds =
        GetPrivateProfileInt(L"Settings", L"fetchingIntervalSeconds", 1, iniFileName);
    GetPrivateProfileString(L"Settings", L"logFile", L"C:\\dir\\default_logs.txt", logFile, 256, iniFileName);
    GetPrivateProfileString(L"Settings", L"dataFile", L"C:\\dir\\default_currency_data.csv", dataFile, 256, iniFileName);
    GetPrivateProfileString(L"Settings", L"currencyCodes", L"USD", wcharCurrencyCodes, 256, iniFileName);

    logFileName.assign(logFile, logFile + wcslen(logFile));
    dataFileName.assign(dataFile, dataFile + wcslen(dataFile));
    currencyCodes.assign(wcharCurrencyCodes, wcharCurrencyCodes + wcslen(wcharCurrencyCodes));
}
