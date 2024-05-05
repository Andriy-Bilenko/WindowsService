#include "helperFunctions.h"

void log(const std::string& logFileName, const std::string& text)
{
    std::ofstream outFile(logFileName, std::ios::app);
    outFile << text << "\r\n";
}

std::string get_current_time()
{
    std::time_t now;
    std::time(&now);

    const size_t bufferSize = 26; // Sufficient to hold the date and time string
    char timeBuffer[bufferSize];

    errno_t err = ctime_s(timeBuffer, bufferSize, &now);
    if (err == 0)
    {
        // Remove the trailing newline character
        if (timeBuffer[bufferSize - 2] == '\n')
        {
            timeBuffer[bufferSize - 2] = '\0';
        }

        return std::string(timeBuffer);
    }
    else
    {
        return "Error: Unable to get current time";
    }
}

void writePrompt()
{
    std::cout << "Parameters:\r\n";
    std::cout << " --install  to install the service.\r\n";
    std::cout << " --remove   to remove the service.\r\n";
    std::cout << " --start   to start the service.\r\n";
    std::cout << " --stop   to stop the service.\r\n";
    std::cout << " --load {filename.ini}   to update service by loading configurations from a file.\r\n";
}

std::string PWSTRToStdString(const PWSTR input) {
    std::wstring wstr = std::wstring(input);
    return std::string(wstr.begin(), wstr.end());
}