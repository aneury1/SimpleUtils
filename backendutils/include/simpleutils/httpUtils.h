#pragma once
#include <map>

#include "socketUtils.h"
namespace simpleutils {
std::string urlEncode(std::string_view value);
std::string urlDecode(std::string_view value, bool plusAsSpace = false);
std::string buildQuery(const std::map<std::string, std::string>& values);
struct HttpUrl {
  std::string scheme, host, target;
  uint16_t port;
};
HttpUrl parseHttpUrl(std::string_view url);
using HttpHeaders = std::map<std::string, std::string>;  // parsed names lowercase
struct HttpResponse {
  int status = 0;
  std::string reason;
  HttpHeaders headers;
  std::string body;
};
HttpResponse parseHttpResponse(std::string_view wire, size_t maximumBody = 8 * 1024 * 1024,
                               bool responseToHead = false);
std::string makeHttpRequest(std::string_view method, const HttpUrl& url,
                            const HttpHeaders& headers = {}, std::string_view body = {});
// HTTP/1.1 plaintext client. No HTTPS, redirects, decompression or connection pooling.
HttpResponse httpRequest(const std::string& url, const std::string& method = "GET",
                         const HttpHeaders& headers = {}, std::string_view body = {},
                         std::chrono::milliseconds timeout = std::chrono::seconds(5),
                         size_t maximumResponse = 8 * 1024 * 1024);
}  // namespace simpleutils
