#include <simpleutils/httpUtils.h>
#include <simpleutils/stringUtils.h>

#include <charconv>
#include <limits>
#include <stdexcept>
namespace simpleutils {
namespace {
int hex(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  throw std::invalid_argument("Invalid URL escape");
}
bool token(std::string_view s) {
  if (s.empty()) return false;
  for (unsigned char c : s)
    if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
          std::string_view("!#$%&'*+-.^_`|~").find(c) != std::string_view::npos))
      return false;
  return true;
}
bool safeValue(std::string_view s) {
  for (unsigned char c : s)
    if (c == 127 || (c < 32 && c != '\t')) return false;
  return true;
}
size_t number(std::string_view s, int base = 10) {
  size_t n = 0;
  auto r = std::from_chars(s.data(), s.data() + s.size(), n, base);
  if (s.empty() || r.ec != std::errc{} || r.ptr != s.data() + s.size())
    throw std::invalid_argument("Invalid HTTP numeric value");
  return n;
}
std::string_view line(std::string_view s, size_t& pos) {
  auto n = s.find("\r\n", pos);
  if (n == s.npos) throw std::invalid_argument("Incomplete HTTP line");
  auto r = s.substr(pos, n - pos);
  pos = n + 2;
  return r;
}
}  // namespace
std::string urlEncode(std::string_view s) {
  const char* h = "0123456789ABCDEF";
  std::string r;
  for (unsigned char c : s) {
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' ||
        c == '_' || c == '.' || c == '~')
      r += c;
    else {
      r += '%';
      r += h[c >> 4];
      r += h[c & 15];
    }
  }
  return r;
}
std::string urlDecode(std::string_view s, bool plus) {
  std::string r;
  for (size_t i = 0; i < s.size(); ++i) {
    if (s[i] == '%') {
      if (i + 2 >= s.size()) throw std::invalid_argument("Truncated URL escape");
      r += static_cast<char>((hex(s[i + 1]) << 4) | hex(s[i + 2]));
      i += 2;
    } else
      r += s[i] == '+' && plus ? ' ' : s[i];
  }
  return r;
}
std::string buildQuery(const std::map<std::string, std::string>& v) {
  std::string r;
  for (const auto& p : v) {
    if (!r.empty()) r += '&';
    r += urlEncode(p.first) + '=' + urlEncode(p.second);
  }
  return r;
}
HttpUrl parseHttpUrl(std::string_view s) {
  auto p = s.find("://");
  if (p == s.npos) throw std::invalid_argument("URL requires scheme");
  HttpUrl u{toLower(s.substr(0, p)), "", "", 0};
  if (u.scheme != "http" && u.scheme != "https")
    throw std::invalid_argument("Unsupported URL scheme");
  u.port = u.scheme == "https" ? 443 : 80;
  s.remove_prefix(p + 3);
  auto end = s.find_first_of("/?#");
  auto authority = s.substr(0, end);
  if (authority.empty() || authority.find('@') != authority.npos)
    throw std::invalid_argument("Invalid URL authority");
  std::string_view host = authority, port;
  if (authority.front() == '[') {
    auto close = authority.find(']');
    if (close == authority.npos || close == 1) throw std::invalid_argument("Invalid IPv6 URL");
    host = authority.substr(1, close - 1);
    auto tail = authority.substr(close + 1);
    if (!tail.empty()) {
      if (tail.front() != ':' || tail.size() == 1) throw std::invalid_argument("Invalid URL port");
      port = tail.substr(1);
    }
  } else {
    auto colon = authority.find(':');
    if (colon != authority.npos) {
      if (authority.find(':', colon + 1) != authority.npos || colon + 1 == authority.size())
        throw std::invalid_argument("IPv6 must be bracketed / empty port");
      host = authority.substr(0, colon);
      port = authority.substr(colon + 1);
    }
  }
  if (host.empty()) throw std::invalid_argument("Empty host");
  for (unsigned char c : host)
    if (c <= 32 || c == 127 || c == '\\' || c == '[' || c == ']')
      throw std::invalid_argument("Invalid host");
  u.host = std::string(host);
  if (!port.empty()) {
    auto n = number(port);
    if (!n || n > 65535) throw std::invalid_argument("Invalid port");
    u.port = static_cast<uint16_t>(n);
  }
  auto rest = end == s.npos ? std::string_view{} : s.substr(end);
  rest = rest.substr(0, rest.find('#'));
  u.target = rest.empty() ? "/" : std::string(rest);
  if (u.target.front() == '?') u.target = '/' + u.target;
  for (unsigned char c : u.target)
    if (c <= 32 || c == 127) throw std::invalid_argument("Unescaped URL control/space");
  return u;
}
std::string makeHttpRequest(std::string_view method, const HttpUrl& u, const HttpHeaders& headers,
                            std::string_view body) {
  if (!token(method) || u.target.empty() || u.target.front() != '/' || u.host.empty())
    throw std::invalid_argument("Invalid request");
  for (unsigned char c : u.target)
    if (c <= 32 || c == 127) throw std::invalid_argument("Invalid target");
  for (unsigned char c : u.host)
    if (c <= 32 || c == 127) throw std::invalid_argument("Invalid host");
  std::string host = u.host.find(':') == u.host.npos ? u.host : '[' + u.host + ']';
  host += ':' + std::to_string(u.port);
  std::string r = std::string(method) + ' ' + u.target + " HTTP/1.1\r\nHost: " + host +
                  "\r\nConnection: close\r\n";
  std::map<std::string, bool> seen;
  for (const auto& p : headers) {
    auto key = toLower(p.first);
    if (!token(p.first) || !safeValue(p.second) || seen[key])
      throw std::invalid_argument("Invalid/duplicate header");
    seen[key] = true;
    if (key == "host" || key == "connection" || key == "content-length" ||
        key == "transfer-encoding")
      throw std::invalid_argument("Reserved HTTP header");
    r += p.first + ": " + p.second + "\r\n";
  }
  r += "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n";
  r += body;
  return r;
}
HttpResponse parseHttpResponse(std::string_view s, size_t max, bool responseToHead) {
  size_t pos = 0;
  auto first = line(s, pos);
  if (pos > 65536) throw std::length_error("HTTP headers too large");
  if (!(startsWith(first, "HTTP/1.1 ") || startsWith(first, "HTTP/1.0 ")) || first.size() < 12)
    throw std::invalid_argument("Invalid HTTP status line");
  if (first.size() > 12 && first[12] != ' ') throw std::invalid_argument("Invalid HTTP reason");
  auto status = number(first.substr(9, 3));
  if (status < 100 || status > 599) throw std::invalid_argument("Invalid HTTP status");
  HttpResponse r;
  r.status = static_cast<int>(status);
  r.reason = first.size() > 12 ? std::string(first.substr(13)) : "";
  if (!safeValue(r.reason)) throw std::invalid_argument("Invalid reason");
  while (true) {
    auto h = line(s, pos);
    if (pos > 65536) throw std::length_error("HTTP headers too large");
    if (h.empty()) break;
    if (pos > 65536) throw std::length_error("HTTP headers too large");
    auto n = h.find(':');
    if (n == h.npos || !token(h.substr(0, n))) throw std::invalid_argument("Invalid header");
    auto key = toLower(h.substr(0, n));
    auto rawValue = h.substr(n + 1);
    if (!safeValue(rawValue)) throw std::invalid_argument("Invalid header value");
    auto value = trim(rawValue);
    if (!safeValue(value)) throw std::invalid_argument("Invalid header value");
    auto old = r.headers.find(key);
    if (old != r.headers.end()) {
      if (key == "content-length" || key == "transfer-encoding")
        throw std::invalid_argument("Duplicate framing header");
      old->second += ", " + value;
    } else
      r.headers[key] = value;
  }
  auto cl = r.headers.find("content-length"), te = r.headers.find("transfer-encoding");
  if (cl != r.headers.end() && te != r.headers.end())
    throw std::invalid_argument("Ambiguous HTTP framing");
  if (cl != r.headers.end()) (void)number(cl->second);
  if (te != r.headers.end() && toLower(te->second) != "chunked")
    throw std::invalid_argument("Unsupported transfer encoding");
  if (responseToHead || (r.status >= 100 && r.status < 200) || r.status == 204 || r.status == 304) {
    if (pos != s.size()) throw std::invalid_argument("Body forbidden for status");
    return r;
  }
  if (te != r.headers.end()) {
    if (toLower(te->second) != "chunked")
      throw std::invalid_argument("Unsupported transfer encoding");
    while (true) {
      auto h = line(s, pos);
      auto sem = h.find(';');
      auto n = number(h.substr(0, sem), 16);
      if (!n) {
        while (true) {
          auto trailer = line(s, pos);
          if (trailer.empty()) break;
          auto colon = trailer.find(':');
          if (colon == trailer.npos || !token(trailer.substr(0, colon)) ||
              !safeValue(trailer.substr(colon + 1)))
            throw std::invalid_argument("Invalid trailer");
          auto name = toLower(trailer.substr(0, colon));
          if (name == "content-length" || name == "transfer-encoding")
            throw std::invalid_argument("Framing trailer forbidden");
        }
        if (pos != s.size()) throw std::invalid_argument("Extra chunked data");
        break;
      }
      if (n > max - r.body.size()) throw std::length_error("HTTP body too large");
      if (n > s.size() - pos || s.size() - pos - n < 2)
        throw std::invalid_argument("Truncated chunk");
      r.body.append(s.substr(pos, n));
      pos += n;
      if (s.substr(pos, 2) != "\r\n") throw std::invalid_argument("Missing chunk terminator");
      pos += 2;
    }
  } else {
    size_t n = cl == r.headers.end() ? s.size() - pos : number(cl->second);
    if (n > max) throw std::length_error("HTTP body too large");
    if (n != s.size() - pos) throw std::invalid_argument("HTTP body length mismatch");
    r.body = std::string(s.substr(pos));
  }
  return r;
}
HttpResponse httpRequest(const std::string& url, const std::string& method, const HttpHeaders& h,
                         std::string_view body, std::chrono::milliseconds timeout, size_t max) {
  auto u = parseHttpUrl(url);
  if (u.scheme != "http")
    throw std::invalid_argument("HTTPS requires a TLS transport; this helper supports HTTP only");
  if (method == "CONNECT") throw std::invalid_argument("HTTP tunnels unsupported");
  auto request = makeHttpRequest(method, u, h, body);
  auto socket = Socket::connectTcp(u.host, u.port, timeout);
  socket.sendAll(request);
  std::string wire;
  while (true) {
    auto p = socket.receive();
    if (p.empty()) break;
    if (p.size() > max - wire.size()) throw std::length_error("HTTP response too large");
    wire += p;
  }
  return parseHttpResponse(wire, max, method == "HEAD");
}
}  // namespace simpleutils
