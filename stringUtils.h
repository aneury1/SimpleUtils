#pragma once
#include <fstream>
#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>

#ifdef __linux__
static const char *FileSeparator ="/";
#elif __WIN32__
static const char *FileSeparator ="\\";
#else
static const char *FileSeparator ="/";
#endif

inline static std::string ReadFile(std::string file)
{
    std::string ret;
    std::fstream stream(file, std::ios::in | std::ios::binary | std::ios::ate);
    int len = stream.tellg();
    stream.seekg(0, std::ios::beg);
    ret.reserve(len);
    std::cout << len << "\n";
    char *buffer = ret.data();
    stream.read(buffer, len);
    stream.close();
    std::cout << stream.gcount() << buffer << "\n";

    return std::string(buffer);
}

inline static std::vector<std::string> splitCSV(const std::string &line, char delimiter = ',')
{
    std::vector<std::string> result;
    std::stringstream ss(line);
    std::string token;

    while (std::getline(ss, token, delimiter))
    {
        result.push_back(token);
    }

    return result;
}

inline static std::vector<std::vector<std::string>> parseCSV(std::string file)
{
    auto content = ReadFile(file);
    std::cout << content << "\n";
    std::stringstream ss(content);
    std::string line;
    std::vector<std::vector<std::string>> ret;
    char delim = '\n';
    while (std::getline(ss, line, delim))
    {
        auto entryLine = splitCSV(line);
        ret.push_back(entryLine);
        line = "";
    }
    return ret;
}


static inline std::string ExtractPathOnly(const std::string filePa){
    
    int pos = filePa.find_last_of(FileSeparator);
    if(pos!= std::string::npos){
        return filePa.substr(0, pos);
    }
  return filePa;
}static inline std::string ExtractFileOnly(const std::string filePa){
    
    int pos = filePa.find_last_of(FileSeparator);
    if(pos!= std::string::npos){
        return filePa.substr(pos+1, filePa.size()-pos);
    }
  return filePa;
}

 static std::vector<uint8_t> ReadBinaryFile(const std::string &filename)
{
    std::ifstream file(filename, std::ios::binary | std::ios::ate); // Open in binary mode, move to the end

    if (!file)
    {

        return {};
    }

    std::streamsize size = file.tellg(); // Get file size
    file.seekg(0, std::ios::beg);        // Rewind to the beginning

    std::vector<uint8_t> buffer(size);      // Allocate buffer
    file.read((char *)buffer.data(), size); // Read file into buffer

    return buffer; // Return file contents
}

static std::string ReadFile(std::string file)
{
    auto fsbuffer = ReadBinaryFile(file);
    auto s = toString(fsbuffer);
    return s;
}