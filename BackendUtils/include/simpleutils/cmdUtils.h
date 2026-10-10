#pragma once
#include <string>
namespace simpleutils {
enum class TTYTerminal { Konsole, Gnome, OtherLinux, MacOS, WindowCMD };
bool IsInstalled(const char* program);
TTYTerminal GetTerminal();  // detection only, never launches a terminal
std::string getLatestClangFormat();
std::string formatWithClangFormat(const std::string& code, const std::string& executable);
}  // namespace simpleutils
