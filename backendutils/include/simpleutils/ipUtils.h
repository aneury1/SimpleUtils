#pragma once
#include <string>
#include <vector>
namespace simpleutils {
struct IPInfo {
  std::string interfaceName, ipAddress, hostName;
};
std::vector<IPInfo> GetLocalIPs2();
std::vector<std::string> GetAllLocalIPs();
bool isIPv4(const std::string& address);
bool isIPv6(const std::string& address);
}  // namespace simpleutils
