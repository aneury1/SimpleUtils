#pragma once
#include <string>

namespace simpleutils {
std::string getCurrentWorkingDirectory();

int getDirectorySize(const std::string& path);

}