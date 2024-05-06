# Windows service application

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

IMPORTANT: if error says `InstallService(): OpenSCManager failed w/err 5` that means you cannot run service unless you're admin. Run it from admin terminal.

## when encountering errors

When service malfunctions pay close attention to error codes, either in terminal right after you ran some `Service.exe` command from the listed above, or to those in service logs. Service logs are located in `C:\dir\default_logs.txt` by default (create dir folder to see), you can specify logs file by loading a new `config.ini`.
Google the error code number if that helps.
