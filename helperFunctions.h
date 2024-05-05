#pragma once
#include <Windows.h>
#include <string>

// function that appends text to logFileName file
void log(const std::string& logFileName, const std::string& text);

// returns a string specifying current date and time (like "Sat May  4 12:35:02 2024")
std::string get_current_time();

// writes usage prompt for current program (Service.exe) to the console
void writePrompt();

// function to convert from inconvenient WinAPI PWSTR to std::string
std::string PWSTRToStdString(PWSTR input);