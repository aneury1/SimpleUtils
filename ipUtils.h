#pragma once 

#include <iostream>
#include <vector>
#include <string>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> 
struct IPInfo {
    std::string interfaceName;
    std::string ipAddress;
    std::string hostName;
};

static inline std::vector<IPInfo> GetLocalIPs2() {
    std::vector<IPInfo> ipList;
    struct ifaddrs *interfaces = nullptr, *ifa = nullptr;

    if (getifaddrs(&interfaces) == -1) {
        perror("getifaddrs");
        return ipList;
    }

    char host[NI_MAXHOST];
    for (ifa = interfaces; ifa != nullptr; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) continue; // Skip interfaces without address

        int family = ifa->ifa_addr->sa_family;
        if (family == AF_INET || family == AF_INET6) {  // IPv4 or IPv6
            void* addr;
            char ipStr[INET6_ADDRSTRLEN];

            if (family == AF_INET) { // IPv4
                addr = &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr;
            } else { // IPv6
                addr = &((struct sockaddr_in6*)ifa->ifa_addr)->sin6_addr;
            }

            inet_ntop(family, addr, ipStr, sizeof(ipStr));

            // Get hostname
            if (getnameinfo(ifa->ifa_addr, (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6),
                            host, NI_MAXHOST, nullptr, 0, NI_NAMEREQD) != 0) {
                snprintf(host, sizeof(host), "Unknown");
            }

            ipList.push_back({ifa->ifa_name, ipStr, host});
        }
    }

    freeifaddrs(interfaces);
    return ipList;
}


static inline wxArrayString GetAllLocalIPs() {
    wxArrayString ips;
    struct addrinfo hints{}, *res, *p;
    hints.ai_family = AF_INET;  // IPv4 only
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, "0", &hints, &res) != 0) {
        return ips;
    }

    for (p = res; p != nullptr; p = p->ai_next) {
        struct sockaddr_in* addr = (struct sockaddr_in*)p->ai_addr;
        ips.Add(wxString::FromUTF8(inet_ntoa(addr->sin_addr)));
    }

    freeaddrinfo(res);
    return ips;
}