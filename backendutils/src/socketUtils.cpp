#include <simpleutils/socketUtils.h>

#include <algorithm>
#include <climits>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <system_error>
#ifndef _WIN32
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#endif
namespace simpleutils {
namespace {
#ifdef _WIN32
int error() { return WSAGetLastError(); }
void startup() {
  static const bool initialized = []() {
    WSADATA w{};
    int e = WSAStartup(MAKEWORD(2, 2), &w);
    if (e) throw std::system_error(e, std::system_category(), "WSAStartup");
    return true;
  }();
  (void)initialized;
}
void blocking(NativeSocket s, bool enabled) {
  u_long mode = enabled ? 0 : 1;
  if (ioctlsocket(s, FIONBIO, &mode))
    throw std::system_error(error(), std::system_category(), "ioctlsocket");
}
#else
int error() { return errno; }
void startup() {}
void blocking(NativeSocket s, bool enabled) {
  int f = fcntl(s, F_GETFL);
  if (f < 0 || fcntl(s, F_SETFL, enabled ? (f & ~O_NONBLOCK) : (f | O_NONBLOCK)) < 0)
    throw std::system_error(errno, std::system_category(), "fcntl");
}
#endif
[[noreturn]] void fail(const char* what) {
  throw std::system_error(error(), std::system_category(), what);
}
void ensure(NativeSocket s) {
  if (s == InvalidSocket) throw std::logic_error("Closed socket");
}
int count(size_t n) { return static_cast<int>(std::min<size_t>(n, INT_MAX)); }
using Addresses = std::unique_ptr<addrinfo, decltype(&freeaddrinfo)>;
Addresses resolve(const std::string& host, uint16_t port, int type, bool passive = false,
                  int family = AF_UNSPEC) {
  if (host.find('\0') != host.npos) throw std::invalid_argument("NUL in hostname");
  startup();
  addrinfo hint{};
  hint.ai_family = family;
  hint.ai_socktype = type;
  hint.ai_flags = passive ? AI_PASSIVE : 0;
  addrinfo* result = nullptr;
  int e = getaddrinfo(host.empty() ? nullptr : host.c_str(), std::to_string(port).c_str(), &hint,
                      &result);
  if (e) throw std::runtime_error("Address resolution failed: " + std::to_string(e));
  return Addresses(result, freeaddrinfo);
}
Socket bindSocket(const std::string& h, uint16_t port, int type) {
  auto addresses = resolve(h, port, type, true);
  for (auto a = addresses.get(); a; a = a->ai_next) {
    Socket s(::socket(a->ai_family, a->ai_socktype, a->ai_protocol));
    if (!s.valid()) continue;
    int yes = 1;
#ifndef _WIN32
    setsockopt(s.nativeHandle(), SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
#else
    setsockopt(s.nativeHandle(), SOL_SOCKET, SO_EXCLUSIVEADDRUSE,
               reinterpret_cast<const char*>(&yes), sizeof(yes));
#endif
    if (::bind(s.nativeHandle(), a->ai_addr, static_cast<int>(a->ai_addrlen)) == 0) return s;
  }
  fail("bind");
}
}  // namespace
Socket::~Socket() { close(); }
Socket::Socket(Socket&& o) noexcept : handle_(o.release()), timeout_(o.timeout_) {}
Socket& Socket::operator=(Socket&& o) noexcept {
  if (this != &o) {
    close();
    handle_ = o.release();
    timeout_ = o.timeout_;
  }
  return *this;
}
NativeSocket Socket::release() noexcept {
  auto h = handle_;
  handle_ = InvalidSocket;
  return h;
}
void Socket::close() noexcept {
  if (valid()) {
#ifdef _WIN32
    closesocket(handle_);
#else
    ::close(handle_);
#endif
    handle_ = InvalidSocket;
  }
}
void Socket::setTimeout(std::chrono::milliseconds t) {
  ensure(handle_);
  if (t.count() <= 0 || t.count() > INT_MAX)
    throw std::invalid_argument("Timeout must be 1..INT_MAX milliseconds");
#ifdef _WIN32
  DWORD v = static_cast<DWORD>(t.count());
  if (setsockopt(handle_, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char*>(&v), sizeof(v)) ||
      setsockopt(handle_, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<char*>(&v), sizeof(v)))
    fail("setsockopt");
#else
  timeval v{static_cast<decltype(timeval::tv_sec)>(t.count() / 1000),
            static_cast<decltype(timeval::tv_usec)>((t.count() % 1000) * 1000)};
  if (setsockopt(handle_, SOL_SOCKET, SO_RCVTIMEO, &v, sizeof(v)) ||
      setsockopt(handle_, SOL_SOCKET, SO_SNDTIMEO, &v, sizeof(v)))
    fail("setsockopt");
#endif
  timeout_ = t;
}
Socket Socket::connectTcp(const std::string& h, uint16_t p, std::chrono::milliseconds t) {
  if (h.empty() || t.count() <= 0 || t.count() > INT_MAX)
    throw std::invalid_argument("Invalid host or timeout");
  auto addresses = resolve(h, p, SOCK_STREAM);
  int last = 0;
  for (auto a = addresses.get(); a; a = a->ai_next) {
    Socket s(::socket(a->ai_family, a->ai_socktype, a->ai_protocol));
    if (!s.valid()) {
      last = error();
      continue;
    }
    blocking(s.handle_, false);
    int ret = ::connect(s.handle_, a->ai_addr, static_cast<int>(a->ai_addrlen));
    if (ret != 0) {
      last = error();
#ifdef _WIN32
      if (last != WSAEWOULDBLOCK && last != WSAEINPROGRESS) continue;
      fd_set writes, errors;
      FD_ZERO(&writes);
      FD_ZERO(&errors);
      FD_SET(s.handle_, &writes);
      FD_SET(s.handle_, &errors);
      timeval tv{static_cast<long>(t.count() / 1000), static_cast<long>((t.count() % 1000) * 1000)};
      ret = select(0, nullptr, &writes, &errors, &tv);
#else
      if (last != EINPROGRESS) continue;
      pollfd fd{s.handle_, POLLOUT, 0};
      do {
        ret = poll(&fd, 1, static_cast<int>(t.count()));
      } while (ret < 0 && errno == EINTR);
#endif
      if (ret <= 0) {
        last = ret < 0 ? error() :
#ifdef _WIN32
                       WSAETIMEDOUT
#else
                       ETIMEDOUT
#endif
            ;
        continue;
      }
      int e = 0;
#ifdef _WIN32
      int len = sizeof(e);
      if (getsockopt(s.handle_, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&e), &len))
        fail("getsockopt");
#else
      socklen_t len = sizeof(e);
      if (getsockopt(s.handle_, SOL_SOCKET, SO_ERROR, &e, &len)) fail("getsockopt");
#endif
      if (e) {
        last = e;
        continue;
      }
    }
    blocking(s.handle_, true);
    s.setTimeout(t);
#ifdef SO_NOSIGPIPE
    int yes = 1;
    setsockopt(s.handle_, SOL_SOCKET, SO_NOSIGPIPE, &yes, sizeof(yes));
#endif
    return s;
  }
  throw std::system_error(last, std::system_category(), "connect");
}
Socket Socket::listenTcp(const std::string& h, uint16_t p, int backlog) {
  if (backlog <= 0) throw std::invalid_argument("Invalid backlog");
  auto s = bindSocket(h, p, SOCK_STREAM);
  if (::listen(s.handle_, backlog)) fail("listen");
  return s;
}
Socket Socket::accept() const {
  ensure(handle_);
  // Windows receive timeouts do not apply to accept(). Explicit readiness wait.
  if (timeout_.count() > 0) {
#ifdef _WIN32
    fd_set readable;
    FD_ZERO(&readable);
    FD_SET(handle_, &readable);
    timeval tv{static_cast<long>(timeout_.count() / 1000),
               static_cast<long>((timeout_.count() % 1000) * 1000)};
    int ready = select(0, &readable, nullptr, nullptr, &tv);
    if (ready == 0) throw std::system_error(WSAETIMEDOUT, std::system_category(), "accept timeout");
#else
    pollfd fd{handle_, POLLIN, 0};
    int ready;
    do {
      ready = poll(&fd, 1, static_cast<int>(timeout_.count()));
    } while (ready < 0 && errno == EINTR);
    if (ready == 0) throw std::system_error(ETIMEDOUT, std::system_category(), "accept timeout");
#endif
    if (ready < 0) fail("accept readiness");
  }

  NativeSocket h;
  do {
    h = ::accept(handle_, nullptr, nullptr);
  } while (h == InvalidSocket && error() ==
#ifdef _WIN32
                                     WSAEINTR
#else
                                     EINTR
#endif
  );
  if (h == InvalidSocket) fail("accept");
#ifdef SO_NOSIGPIPE
  int yes = 1;
  setsockopt(h, SOL_SOCKET, SO_NOSIGPIPE, &yes, sizeof(yes));
#endif
  return Socket(h);
}
uint16_t Socket::localPort() const {
  ensure(handle_);
  sockaddr_storage addr{};
#ifdef _WIN32
  int n = sizeof(addr);
#else
  socklen_t n = sizeof(addr);
#endif
  if (getsockname(handle_, reinterpret_cast<sockaddr*>(&addr), &n)) fail("getsockname");
  return ntohs(addr.ss_family == AF_INET ? reinterpret_cast<sockaddr_in*>(&addr)->sin_port
                                         : reinterpret_cast<sockaddr_in6*>(&addr)->sin6_port);
}
void Socket::sendAll(std::string_view bytes) const {
  ensure(handle_);
  size_t done = 0;
  while (done < bytes.size()) {
#ifdef MSG_NOSIGNAL
    constexpr int flags = MSG_NOSIGNAL;
#else
    constexpr int flags = 0;
#endif
    auto n = ::send(handle_, bytes.data() + done, count(bytes.size() - done), flags);
    if (n < 0) {
#ifdef _WIN32
      if (error() == WSAEINTR) continue;
#else
      if (errno == EINTR) continue;
#endif
      fail("send");
    }
    if (!n) throw std::runtime_error("Zero-byte send");
    done += static_cast<size_t>(n);
  }
}
std::string Socket::receive(size_t max) const {
  ensure(handle_);
  if (!max || max > INT_MAX) throw std::invalid_argument("Invalid receive size");
  std::string b(max, '\0');
  decltype(::recv(handle_, b.data(), count(max), 0)) n;
  do {
    n = ::recv(handle_, b.data(), count(max), 0);
  } while (n < 0 && error() ==
#ifdef _WIN32
                        WSAEINTR
#else
                        EINTR
#endif
  );
  if (n < 0) fail("recv");
  b.resize(static_cast<size_t>(n));
  return b;
}
std::string Socket::receiveExact(size_t n) const {
  ensure(handle_);
  std::string b;
  while (b.size() < n) {
    auto part = receive(std::min<size_t>(65536, n - b.size()));
    if (part.empty()) throw std::runtime_error("Unexpected socket EOF");
    b += part;
  }
  return b;
}
Socket Socket::bindUdp(const std::string& h, uint16_t p) { return bindSocket(h, p, SOCK_DGRAM); }
void Socket::sendTo(std::string_view b, const std::string& h, uint16_t p) const {
  ensure(handle_);
  if (b.size() > 65507) throw std::length_error("UDP payload too large");
  sockaddr_storage local{};
#ifdef _WIN32
  int l = sizeof(local);
#else
  socklen_t l = sizeof(local);
#endif
  if (getsockname(handle_, reinterpret_cast<sockaddr*>(&local), &l)) fail("getsockname");
  auto a = resolve(h, p, SOCK_DGRAM, false, local.ss_family);
  auto n =
      ::sendto(handle_, b.data(), count(b.size()), 0, a->ai_addr, static_cast<int>(a->ai_addrlen));
  if (n < 0) fail("sendto");
  if (static_cast<size_t>(n) != b.size()) throw std::runtime_error("Partial datagram");
}
Socket::Datagram Socket::receiveFrom(size_t max) const {
  ensure(handle_);
  if (!max || max > 65536) throw std::invalid_argument("Invalid UDP buffer size");
  sockaddr_storage a{};
#ifdef _WIN32
  int l = sizeof(a);
#else
  socklen_t l = sizeof(a);
#endif
  std::string b(65536, '\0');
  auto n = ::recvfrom(handle_, b.data(), static_cast<int>(b.size()), 0,
                      reinterpret_cast<sockaddr*>(&a), &l);
  if (n < 0) fail("recvfrom");
  if (static_cast<size_t>(n) > max) throw std::length_error("Datagram exceeds receive limit");
  b.resize(n);
  char h[NI_MAXHOST], port[NI_MAXSERV];
  if (getnameinfo(reinterpret_cast<sockaddr*>(&a), l, h, sizeof(h), port, sizeof(port),
                  NI_NUMERICHOST | NI_NUMERICSERV))
    throw std::runtime_error("Datagram address error");
  return {b, h, static_cast<uint16_t>(std::stoi(port))};
}
}  // namespace simpleutils
