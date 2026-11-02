#include <simpleutils/ipUtils.h>
#include <simpleutils/socketUtils.h>

#include <algorithm>
#include <cerrno>
#include <memory>
#include <stdexcept>
#include <system_error>
#ifdef _WIN32
#include <iphlpapi.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#endif
namespace simpleutils {
bool isIPv4(const std::string& s) {
  if (s.find('\0') != s.npos) return false;
  in_addr a{};
  return inet_pton(AF_INET, s.c_str(), &a) == 1;
}
bool isIPv6(const std::string& s) {
  if (s.find('\0') != s.npos) return false;
  in6_addr a{};
  return inet_pton(AF_INET6, s.c_str(), &a) == 1;
}
std::vector<IPInfo> GetLocalIPs2() {
  std::vector<IPInfo> r;
#ifdef _WIN32
  ULONG n = 15000;
  std::vector<unsigned char> buffer(n);
  ULONG e = ERROR_BUFFER_OVERFLOW;
  for (int attempt = 0; attempt < 3 && e == ERROR_BUFFER_OVERFLOW; ++attempt) {
    buffer.resize(n);
    e = GetAdaptersAddresses(
        AF_UNSPEC, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER,
        nullptr, reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data()), &n);
  }
  if (e != NO_ERROR) throw std::runtime_error("GetAdaptersAddresses failed");
  for (auto p = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data()); p; p = p->Next)
    for (auto a = p->FirstUnicastAddress; a; a = a->Next) {
      char text[INET6_ADDRSTRLEN]{};
      auto addr = a->Address.lpSockaddr;
      int family = addr->sa_family;
      if (family != AF_INET && family != AF_INET6) continue;
      const void* raw = family == AF_INET
                            ? static_cast<void*>(&reinterpret_cast<sockaddr_in*>(addr)->sin_addr)
                            : static_cast<void*>(&reinterpret_cast<sockaddr_in6*>(addr)->sin6_addr);
      if (inet_ntop(family, raw, text, sizeof(text))) r.push_back({p->AdapterName, text, ""});
    }
#else
  ifaddrs* addresses = nullptr;
  if (getifaddrs(&addresses)) throw std::system_error(errno, std::system_category(), "getifaddrs");
  std::unique_ptr<ifaddrs, decltype(&freeifaddrs)> owner(addresses, freeifaddrs);
  for (auto p = addresses; p; p = p->ifa_next) {
    if (!p->ifa_addr) continue;
    auto family = p->ifa_addr->sa_family;
    if (family != AF_INET && family != AF_INET6) continue;
    char text[INET6_ADDRSTRLEN]{};
    const void* raw =
        family == AF_INET
            ? static_cast<void*>(&reinterpret_cast<sockaddr_in*>(p->ifa_addr)->sin_addr)
            : static_cast<void*>(&reinterpret_cast<sockaddr_in6*>(p->ifa_addr)->sin6_addr);
    if (inet_ntop(family, raw, text, sizeof(text))) r.push_back({p->ifa_name, text, ""});
  }
#endif
  return r;
}
std::vector<std::string> GetAllLocalIPs() {
  std::vector<std::string> r;
  for (const auto& p : GetLocalIPs2())
    if (std::find(r.begin(), r.end(), p.ipAddress) == r.end()) r.push_back(p.ipAddress);
  return r;
}
}  // namespace simpleutils
