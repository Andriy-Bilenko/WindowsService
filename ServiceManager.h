#pragma once
#include <Windows.h>
#include <string>
#include <vector>

class ServiceManager
{
public:
    ServiceManager() = default;

    // registering service with Service Control Manager
    // function not intended to be used by users, only by SCM
    int registerServiceWithSCM(PWSTR pszServiceName);

    // install the service and write hardcoded configurations to its registry
    int installService(PWSTR pszServiceName,
        PWSTR pszDisplayName,
        DWORD dwStartType,
        PWSTR pszDependencies,
        PWSTR pszAccount,
        PWSTR pszPassword);

    // removing service
    int removeService(PWSTR pszServiceName);

    // starting service
    int startService(PWSTR pszServiceName);

    // stopping service. generally recommended before removing it
    int stopService(PWSTR pszServiceName);

    // load configs from .ini file and writing to the service registry
    int loadConfigsFromFile(PWSTR pszServiceName, PWSTR iniFileName);

private:
    // writing to registry fetchingIntervalSeconds, logFileName, dataFileName and currencyCodes
    int writeToRegistry(const int& fetchingIntervalSeconds, const std::string& logFileName, const std::string& dataFileName, const std::string& currencyCodes, const std::string& serviceName);

    // getting variable data from .ini files
    void parseIni(int& fetchingIntervalSeconds, std::string& logFileName, std::string& dataFileName, std::string& currencyCodes, PWSTR iniFileName);
};