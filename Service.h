#pragma once

#include <vector>
#include <thread>
#include <cpprest/http_client.h>
#include <cpprest/filestream.h>
#include <cpprest/uri.h>
#include <cpprest/json.h>
#include "ServiceBase.h"
#include "helperFunctions.h"
#include "ServiceManager.h"

// service configurations for NBU API
#define NBU_HOSTNAME L"https://bank.gov.ua"
#define NBU_QUERY L"/NBUStatService/v1/statdirectory/exchange?json"

// default service configurations
#define DEFAULT_LOG_FILENAME "C:\\dir\\default_logs.txt"
#define DEFAULT_DATA_FILENAME "C:\\dir\\default_currency_data.csv"
#define DEFAULT_FETCHING_INTERVAL_SECONDS 2
#define DEFAULT_CURRENCY_CODES "USD, EUR"

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
    static std::string& getCurrencyCodes();
    static std::string& getLogFileName();
    static std::string& getDataFileName();

    // configuration variables setters
    static void setFetchingIntervalSeconds(const int& interval);
    static void setCurrencyCodes(const std::string & currencyCodes);
    static void setLogFileName(const std::string& name);
    static void setDataFileName(const std::string& name);

protected:
    virtual void OnStart(DWORD dwArgc, PWSTR* pszArgv);

    virtual void OnStop();

    // function to fetch currency data from NBU API (https://bank.gov.ua/NBUStatService/v1/statdirectory/exchange?json)
    // writes fetched data to g_dataFileName every g_fetchingIntervalSeconds seconds
    void fetch();

    // function to register RegistryChangeCallback callback
    // returns 0 if successful, -1 otherwise
    int monitorRegistry();

private:
    BOOL m_serviceStopping;
    HANDLE m_hStoppedEvent;

    // static service configuration variables
    // exactly static to be updated in RegistryChangeCallback in Service.cpp
    static int g_fetchingIntervalSeconds;
    static std::string g_currencyCodes;
    static std::string g_logFileName;
    static std::string g_dataFileName;
};