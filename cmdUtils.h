#include <iostream>
#include <sstream>
#include <cstdio>
#include <memory>
#include <vector>
#include <string>
#include <algorithm>
#include <filesystem>
#include <fstream>

enum class TTYTerminal{
    Konsole,
    Gnome,
    OtherLinux,
    MacOS,
    WindowCMD
};

static bool IsInstalled(const char* terminal) {
    std::string cmd = "which ";
    cmd += terminal;
    cmd += " > /dev/null 2>&1"; // Suppress output
    return system(cmd.c_str()) == 0;
}
static TTYTerminal GetTerminal(){
    system("x-terminal-emulator &");
    if (IsInstalled("konsole")) {
        std::cout << "Konsole is installed.\n";
        return TTYTerminal::Konsole;
        
    } else if (IsInstalled("gnome-terminal")) {
        std::cout << "GNOME Terminal is installed.\n";
        return TTYTerminal::Gnome;
    } else {
        std::cout << "Neither Konsole nor GNOME Terminal is installed.\n";
    }

    return TTYTerminal::OtherLinux;
}

static inline std::string getLatestClangFormat() {
    std::vector<std::string> possible_versions = {
        "clang-format", "clang-format-17", "clang-format-16", "clang-format-15",
        "clang-format-14", "clang-format-13", "clang-format-12", "clang-format-11"
    };

    for (const auto& version : possible_versions) {
        if (std::filesystem::exists(std::string("/usr/bin/") + version)) {
            return version;
        }
    }

    return "";
}



static inline std::string formatWithClangFormat(const std::string& code, const std::string& clangVersion) {
    // Create a temporary file
    std::string filename = "/tmp/temp_code.cpp";
    std::ofstream file(filename);
    file << code;
    file.close();

    // Run clang-format
    std::string command = clangVersion+ " -i " + filename;
    system(command.c_str());

    // Read formatted code
    std::ifstream formattedFile(filename);
    std::stringstream buffer;
    buffer << formattedFile.rdbuf();
    
    return buffer.str();
}