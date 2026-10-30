#pragma once
#include <regex>
#include <string>
namespace simpleutils {
inline bool validateExpression(const std::string& text, const std::string& pattern) {
  try {
    return std::regex_match(text, std::regex(pattern));
  } catch (const std::regex_error&) {
    return false;
  }
}
}  // namespace simpleutils
