#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
namespace simpleutils {
#ifdef _WIN32
inline constexpr const char* FileSeparator = "\\";
#else
inline constexpr const char* FileSeparator = "/";
#endif
std::string toString(const std::vector<uint8_t>& bytes);
std::vector<uint8_t> toBytes(std::string_view text);
std::string ReadFile(const std::string& path);
std::vector<uint8_t> ReadBinaryFile(const std::string& path);
void WriteFile(const std::string& path, std::string_view data);
std::string ExtractPathOnly(const std::string& path);
std::string ExtractFileOnly(const std::string& path);
std::string trim(std::string_view text);
std::string toLower(std::string_view text);
std::string toUpper(std::string_view text);
bool startsWith(std::string_view text, std::string_view prefix);
bool endsWith(std::string_view text, std::string_view suffix);
std::string replaceAll(std::string_view text, std::string_view from, std::string_view to);
std::vector<std::string> split(std::string_view text, std::string_view delimiter,
                               bool keepEmpty = true);
std::string join(const std::vector<std::string>& parts, std::string_view delimiter);
std::vector<std::string> splitCSV(const std::string& line, char delimiter = ',');
std::vector<std::vector<std::string>> parseCSV(const std::string& path);
std::vector<std::vector<std::string>> parseCSVText(std::string_view text, char delimiter = ',');
std::string toHex(const std::vector<uint8_t>& bytes, bool uppercase = false);
std::vector<uint8_t> fromHex(std::string_view hex);
size_t find(const char* text, const char* needle);
}  // namespace simpleutils
