#include <windows.h>
#include <iostream>
#include "ServiceManager.h"
#include "helperFunctions.h"

#define SERVICE_NAME L"AServiceName"
#define SERVICE_DISPLAY_NAME L"A Unique Service Name"
#define SERVICE_START_TYPE SERVICE_DEMAND_START
#define SERVICE_DEPENDENCIES L""
#define SERVICE_ACCOUNT L"NT AUTHORITY\\LocalService" // anonimous limited privileges system account
#define SERVICE_PASSWORD NULL

// main entry point
int main(int argc, char** argv)
{
    PWSTR serviceName = const_cast<PWSTR>(SERVICE_NAME);
    PWSTR serviceDisplayName = const_cast<PWSTR>(SERVICE_DISPLAY_NAME);
    DWORD serviceStartType = SERVICE_DEMAND_START;
    PWSTR serviceDependencies = const_cast<PWSTR>(SERVICE_DEPENDENCIES);
    PWSTR serviceAccount = const_cast<PWSTR>(SERVICE_ACCOUNT); 
    PWSTR servicePassword = SERVICE_PASSWORD;

    ServiceManager serviceMan;

    if (argc == 3 && std::strcmp(argv[1], "--load") == 0)
    {
        const int MAX_FILENAME_LENGTH = 256;
        // get argv[2] length
        size_t len;
        mbstowcs_s(&len, nullptr, 0, argv[2], 0);
        if (len < MAX_FILENAME_LENGTH) {
            wchar_t warg2[MAX_FILENAME_LENGTH];
            mbstowcs_s(&len, warg2, argv[2], strlen(argv[2]) + 1);
            serviceMan.loadConfigsFromFile(serviceName, warg2);
        }
        else {
            std::cerr << ".ini file name too long. stopping.\r\n";
        }
    }
    else if (argc == 2)
    {
        if (std::strcmp(argv[1], "--install") == 0)
        {
            serviceMan.installService(
                serviceName,
                serviceDisplayName,
                serviceStartType,
                serviceDependencies,
                serviceAccount,
                servicePassword);
        }
        else if (std::strcmp(argv[1], "--remove") == 0)
        {
            serviceMan.removeService(serviceName);
        }
        else if (std::strcmp(argv[1], "--start") == 0)
        {
            serviceMan.startService(serviceName);
        }
        else if (std::strcmp(argv[1], "--stop") == 0)
        {
            serviceMan.stopService(serviceName);
        }
        else
        {
            writePrompt();
        }
    }
    else
    {
        serviceMan.registerServiceWithSCM(serviceName);
        writePrompt();
    }

    return 0;
}
