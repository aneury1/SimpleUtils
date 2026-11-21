
#include <filesystem>
#include <string>
#include <dirent.h>
#include <sys/stat.h>
 
#include <unistd.h>

namespace simpleutils {

std::string getCurrentWorkingDirectory() { return std::filesystem::current_path().string(); }

int getDirectorySize(const std::string& path) {
        DIR* dir = opendir(path.c_str());
    if (!dir) {
        return 0; // Handle error
    }

    int totalSize = 0;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string entryPath = path + "/" + entry->d_name;
        if (entry->d_type == DT_REG) { // Regular file
            struct stat fileStat;
            if (stat(entryPath.c_str(), &fileStat) == 0) {
                totalSize += static_cast<int>(fileStat.st_size);
            }
        } else if (entry->d_type == DT_DIR) { // Directory
            if (entry->d_name[0] != '.') { // Skip . and ..
                std::string subDirPath = path + "/" + entry->d_name;
                totalSize += getDirectorySize(subDirPath);
            }
        }
    }

    closedir(dir);
    return totalSize;
}

}  // namespace simpleUtils