# Currency Fetcher Service

Windows service application in C++ to fetch currency rates using data from the National Bank of Ukraine (NBU) API

## overview

-   `main.cpp` - console application entry point
-   `ServiceBase.h` and `ServiceBase.cpp` define the base class for our service. Has basic service features like registration with Service Control Manager (SCM), singleton architecture, ServiceMain, ServiceCtrlHandler and other functions that react to SCM directives.
-   `Service.h` and `Service.cpp` include main feature functionalities like fetching data from API and monitoring registers for changes to update the service behaviour during the runtime.
-   `ServiceManager.h` and `ServiceManager.cpp` includes functions that get called from `main.cpp` for managing service. Those functions include:
    -   RegisterServiceWithSCM - called when you run `Service.exe` without parameters. Intended to be run by SCM.
    -   InstallService - installs the service and writes to service registry default configurations
    -   RemoveService - removes the service
    -   StartService - starts the installed service (fetching data from API and monitoring registry changes)
    -   StopService - stops the installed service
    -   LoadConfigsFromFile - reads configurations from .ini file and puts them to the service registry on demand

## configuration and use

usage:
Make sure to have `cpprest_2_10.dll` in the same folder as executable and run under admin terminal.
```bash
Service.exe --install # required
Service.exe --load {filename.ini} # optional. SPECIFY FULL PATH TO .ini FILE, like "C:\\dir\\config.ini".
Service.exe --start # to actually start the service
#service started, writing logs and fetched data to respective files
Service.exe --load {filename.ini} # optional. SPECIFY FULL PATH TO .ini FILE. can load configurations in runtime and apply them instantly
Service.exe --stop # to stop the service. no longer fetches data / writes to files
Service.exe --remove # to get rid of the service from the computer. it is recommended to stop the service (if it was started) before removing it.
```

for example configuration file see `example_config.ini`

## dependencies

CppRestSdk from https://github.com/microsoft/cpprestsdk, a Windows-native C++ REST SDK developed by Microsoft for modern asynchronous C++ API design and calls.

Compile project with Microsoft Visual Studio, otherwise make sure to include `cpprest_2_10.dll` to the directory with `Service.exe`.

## troubleshooting

When service malfunctions pay close attention to error codes, either in terminal or service logs. Service logs are located in `C:\dir\default_logs.txt` by default (create `C:\dir\` or specify logs and data file by loading a new `config.ini`).

- if error says `InstallService(): OpenSCManager failed w/err 5` make sure you run it from admin terminal
- if error says `StartService(): Failed to start service. Error code: 2` move folder from Downloads to some other folder on disk
- if you see no outputs, like:
    ```
    PS C:\codes\abz.agency test task for C++ developer> .\Service.exe --install
    PS C:\codes\abz.agency test task for C++ developer> .\Service.exe --start
    PS C:\codes\abz.agency test task for C++ developer> .\Service.exe
    PS C:\codes\abz.agency test task for C++ developer>
    ```
    instead of
  ```
    PS C:\codes\abz.agency test task for C++ developer> .\Service.exe
    RegisterServiceWithSCM: Service failed to run w/err 1063
    this function should be ran by Service Control Manager and not from command line
    Parameters:
     --install  to install the service.
     --remove   to remove the service.
     --start   to start the service.
     --stop   to stop the service.
     --load {filename.ini}   to update service by loading configurations from a file.
    PS C:\codes\abz.agency test task for C++ developer> .\Service.exe --install
    AServiceName is installed
    PS C:\codes\abz.agency test task for C++ developer> .\Service.exe --start
    Service started successfully.
    PS C:\codes\abz.agency test task for C++ developer> .\Service.exe --load C:\example_config.ini
    PS C:\codes\abz.agency test task for C++ developer>
    ```
  make sure you have `cpprest_2_10.dll` in the same folder as `Service.exe`.

Google the error code number if nothing helps.


If you ran into some issues by any chance or need to contact the developer, it would be great to recieve your valuable feedback on email: *bilenko.a.uni@gmail.com*.

<div align="right">
<table><td>
<a href="#start-of-content">↥ Scroll to top</a>
</td></table>
</div>

