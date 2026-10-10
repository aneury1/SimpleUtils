#pragma once
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#endif
namespace simpleutils {
#ifdef _WIN32
using NativeSocket = SOCKET;
constexpr NativeSocket InvalidSocket = INVALID_SOCKET;
#else
using NativeSocket = int;
constexpr NativeSocket InvalidSocket = -1;
#endif
// Raw OS socket helpers, not privileged IP packet injection. TCP and UDP, IPv4/IPv6.
class Socket {
  NativeSocket handle_ = InvalidSocket;
  std::chrono::milliseconds timeout_{0};

 public:
  explicit Socket(NativeSocket handle = InvalidSocket) noexcept : handle_(handle) {}
  ~Socket();
  Socket(const Socket&) = delete;
  Socket& operator=(const Socket&) = delete;
  Socket(Socket&& other) noexcept;
  Socket& operator=(Socket&& other) noexcept;
  NativeSocket nativeHandle() const noexcept { return handle_; }
  bool valid() const noexcept { return handle_ != InvalidSocket; }
  void close() noexcept;
  NativeSocket release() noexcept;
  void setTimeout(std::chrono::milliseconds timeout);
  static Socket connectTcp(const std::string& host, uint16_t port,
                           std::chrono::milliseconds timeout = std::chrono::seconds(5));
  static Socket listenTcp(const std::string& host, uint16_t port, int backlog = 16);
  Socket accept() const;
  uint16_t localPort() const;
  void sendAll(std::string_view bytes) const;
  std::string receive(size_t maximum = 4096) const;  // empty only on TCP EOF
  std::string receiveExact(size_t count) const;      // throws on early EOF
  static Socket bindUdp(const std::string& host, uint16_t port);
  void sendTo(std::string_view bytes, const std::string& host, uint16_t port) const;
  struct Datagram {
    std::string data;
    std::string host;
    uint16_t port;
  };
  Datagram receiveFrom(size_t maximum = 65536) const;
};
}  // namespace simpleutils
