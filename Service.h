#pragma once
#include "ServiceBase.h"
#include <vector>

class Service : public ServiceBase
{
public:
    Service(PWSTR pszServiceName,
        BOOL fCanStop = TRUE,
        BOOL fCanShutdown = TRUE,
        BOOL fCanPauseContinue = FALSE);

    virtual ~Service(void);

    // configuration variables getters
    static int& getFetchingIntervalSeconds();
    static std::vector<std::string>& getCurrencyCodes();
    static std::string& getLogFileName();
    static std::string& getDataFileName();

    // configuration variables setters
    static void setFetchingIntervalSeconds(int& interval);
    static void setCurrencyCodes(std::vector<std::string>& currencyCodes);
    static void setLogFileName(std::string& name);
    static void setDataFileName(std::string& name);

protected:
    virtual void OnStart(DWORD dwArgc, PWSTR* pszArgv);

    virtual void OnStop();

    // fucntion to fetch currency data from NBU API (https://bank.gov.ua/NBUStatService/v1/statdirectory/exchange?json)
    // writes fetched data to g_dataFileName every g_fetchingIntervalSeconds seconds
    void fetch();

    // function to register RegistryChangeCallback callback
    int monitorRegistry();

private:
    BOOL m_serviceStopping;
    HANDLE m_hStoppedEvent;

    // static service configuration variables
    // exactly static to be updated in RegistryChangeCallback in .cpp
    static int g_fetchingIntervalSeconds;
    static std::vector<std::string> g_currencyCodes;
    static std::string g_logFileName;
    static std::string g_dataFileName;
};