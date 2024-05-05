#pragma once

#include <Windows.h>
#include <iostream>
#include <string>
#include <vector>
#include "Service.h"
#include "helperFunctions.h"

#define REGISTRY_KEY_DIR "SYSTEM\\CurrentControlSet\\Services\\"
#define SECTION_SETTINGS "Settings"
// registry key names. same as config.ini key names
#define REGISTRY_VALUE_FETCHINTERVAL "FetchingIntervalSeconds"
#define REGISTRY_VALUE_LOGFILE "LogFileName"
#define REGISTRY_VALUE_DATAFILE "DataFileName"
#define REGISTRY_VALUE_CURRENCY_CODES "CurrencyCodes"

class ServiceManager
{
public:
    ServiceManager() = default;

    // registering service with Service Control Manager
    // function not intended to be used by users, only by SCM
    // returns 0 if successful, -1 otherwise
    int registerServiceWithSCM(PWSTR pszServiceName);

    // install the service and write hardcoded configurations to its registry
    // returns 0 if successful, -1 otherwise
    int installService(PWSTR pszServiceName,
        PWSTR pszDisplayName,
        DWORD dwStartType,
        PWSTR pszDependencies,
        PWSTR pszAccount,
        PWSTR pszPassword);

    // removing service
    // returns 0 if successful, -1 otherwise
    int removeService(PWSTR pszServiceName);

    // starting service
    // returns 0 if successful, -1 otherwise
    int startService(PWSTR pszServiceName);

    // stopping service. generally recommended before removing it
    // returns 0 if successful, -1 otherwise
    int stopService(PWSTR pszServiceName);

    // load configs from .ini file and writing to the service registry
    // returns 0 if successful, -1 otherwise
    int loadConfigsFromFile(PWSTR pszServiceName, PWSTR iniFileName);

private:
    // writing to registry fetchingIntervalSeconds, logFileName, dataFileName and currencyCodes
    // returns 0 if successful, -1 otherwise
    int writeToRegistry(const int& fetchingIntervalSeconds, const std::string& logFileName, const std::string& dataFileName, const std::string& currencyCodes, const std::string& serviceName);

    // getting variable data from .ini files
    void parseIni(int& fetchingIntervalSeconds, std::string& logFileName, std::string& dataFileName, std::string& currencyCodes, PWSTR iniFileName);
};