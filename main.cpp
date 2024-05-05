#pragma region Includes
#include <windows.h>
#include <iostream>
#include "ServiceBase.h"
#include "Service.h"
#include "ServiceManager.h"
#include "helperFunctions.h"
#pragma endregion

// main entry point
int main(int argc, char** argv)
{
    PWSTR SERVICE_NAME = const_cast<PWSTR>(L"AServiceName");
    PWSTR SERVICE_DISPLAY_NAME = const_cast<PWSTR>(L"A Unique Service Name");
    DWORD SERVICE_START_TYPE = SERVICE_DEMAND_START;
    PWSTR SERVICE_DEPENDENCIES = const_cast<PWSTR>(L"");
    PWSTR SERVICE_ACCOUNT = const_cast<PWSTR>(L"NT AUTHORITY\\LocalService"); // anonimous limited privileges system account
    PWSTR SERVICE_PASSWORD = NULL;

    ServiceManager serviceMan;

    if (argc == 3 && std::strcmp(argv[1], "--load") == 0)
    {
        size_t len;
        wchar_t warg2[256];
        mbstowcs_s(&len, warg2, argv[2], strlen(argv[2]) + 1);
        serviceMan.loadConfigsFromFile(SERVICE_NAME, warg2);
    }
    else if (argc == 2)
    {
        if (std::strcmp(argv[1], "--install") == 0)
        {
            serviceMan.installService(
                SERVICE_NAME,
                SERVICE_DISPLAY_NAME,
                SERVICE_START_TYPE,
                SERVICE_DEPENDENCIES,
                SERVICE_ACCOUNT,
                SERVICE_PASSWORD);
        }
        else if (std::strcmp(argv[1], "--remove") == 0)
        {
            serviceMan.removeService(SERVICE_NAME);
        }
        else if (std::strcmp(argv[1], "--start") == 0)
        {
            serviceMan.startService(SERVICE_NAME);
        }
        else if (std::strcmp(argv[1], "--stop") == 0)
        {
            serviceMan.stopService(SERVICE_NAME);
        }
        else
        {
            writePrompt();
        }
    }
    else
    {
        serviceMan.registerServiceWithSCM(SERVICE_NAME);
        writePrompt();
    }

    return 0;
}
