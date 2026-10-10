#include "test_support.h"
using namespace simpleutils;
using namespace std::chrono_literals;
TEST(URL, EncodeBinaryRoundtrip) {
  std::string all;
  for (int i = 0; i < 256; ++i) all += static_cast<char>(i);
  EXPECT_EQ(urlDecode(urlEncode(all)), all);
  EXPECT_EQ(urlEncode("a b/+"), "a%20b%2F%2B");
  EXPECT_EQ(urlDecode("a+b", true), "a b");
  EXPECT_EQ(urlDecode("a+b"), "a+b");
  EXPECT_EQ(buildQuery({{"a b", "x+y"}, {"z", ""}}), "a%20b=x%2By&z=");
}
class BadEscape : public testing::TestWithParam<const char*> {};
TEST_P(BadEscape, Reject) { EXPECT_THROW(urlDecode(GetParam()), std::invalid_argument); }
INSTANTIATE_TEST_SUITE_P(Malformed, BadEscape, testing::Values("%", "%0", "%xz", "abc%gg"));
TEST(URL, Parse) {
  auto u = parseHttpUrl("http://example.com:8080/a?x=1#fragment");
  EXPECT_EQ(u.host, "example.com");
  EXPECT_EQ(u.port, 8080);
  EXPECT_EQ(u.target, "/a?x=1");
  u = parseHttpUrl("https://[::1]/");
  EXPECT_EQ(u.host, "::1");
  EXPECT_EQ(u.port, 443);
  EXPECT_EQ(parseHttpUrl("http://host?x").target, "/?x");
  EXPECT_EQ(parseHttpUrl("http://host").target, "/");
}
class BadUrl : public testing::TestWithParam<const char*> {};
TEST_P(BadUrl, Reject) { EXPECT_THROW(parseHttpUrl(GetParam()), std::invalid_argument); }
INSTANTIATE_TEST_SUITE_P(Malformed, BadUrl,
                         testing::Values("host", "ftp://x", "http://", "http://user@host",
                                         "http://x:0", "http://x:65536", "http://x:", "http://::1",
                                         "http://[::1", "http://x/a b", "http://x\r\nX:a",
                                         "http://[::1]:", "http://:80"));
TEST(HTTP, RequestBinaryAndHeaders) {
  auto u = parseHttpUrl("http://[::1]:8000/a");
  auto r = makeHttpRequest("POST", u, {{"X-Test", "yes"}}, std::string("a\0b", 3));
  EXPECT_NE(r.find("Host: [::1]:8000\r\n"), r.npos);
  EXPECT_NE(r.find("Content-Length: 3\r\n"), r.npos);
  EXPECT_EQ(r.substr(r.size() - 3), std::string("a\0b", 3));
  EXPECT_THROW(makeHttpRequest("GET\r\nX", u), std::invalid_argument);
  EXPECT_THROW(makeHttpRequest("GET", u, {{"X", "a\r\nY:b"}}), std::invalid_argument);
  EXPECT_THROW(makeHttpRequest("GET", u, {{"Content-Length", "9"}}), std::invalid_argument);
  EXPECT_THROW(makeHttpRequest("GET", u, {{"Host", "evil"}}), std::invalid_argument);
  EXPECT_THROW(makeHttpRequest("GET", u, {{"Transfer-Encoding", "chunked"}}),
               std::invalid_argument);
  EXPECT_THROW(makeHttpRequest("GET", u, {{"X", "a"}, {"x", "b"}}), std::invalid_argument);
}
TEST(HTTP, ParseLengthAndCloseDelimited) {
  auto r = parseHttpResponse("HTTP/1.1 200 OK\r\nContent-Length: 3\r\nX-A: a\r\nX-A: b\r\n\r\nabc");
  EXPECT_EQ(r.status, 200);
  EXPECT_EQ(r.reason, "OK");
  EXPECT_EQ(r.headers["x-a"], "a, b");
  EXPECT_EQ(r.body, "abc");
  EXPECT_EQ(parseHttpResponse("HTTP/1.0 404 Not Found\r\n\r\nmissing").body, "missing");
  EXPECT_EQ(parseHttpResponse("HTTP/1.1 204 No Content\r\n\r\n").body, "");
}
TEST(HTTP, Chunked) {
  auto r = parseHttpResponse(
      "HTTP/1.1 200 OK\r\nTransfer-Encoding: "
      "chunked\r\n\r\n3;foo=bar\r\nabc\r\n2\r\nde\r\n0\r\nX-T: v\r\n\r\n");
  EXPECT_EQ(r.body, "abcde");
  EXPECT_THROW(parseHttpResponse("HTTP/1.1 200 OK\r\n\r\nabc", 2), std::length_error);
}
class BadHttp : public testing::TestWithParam<const char*> {};
TEST_P(BadHttp, Reject) { EXPECT_ANY_THROW(parseHttpResponse(GetParam())); }
INSTANTIATE_TEST_SUITE_P(
    Malformed, BadHttp,
    testing::Values(
        "", "HTTP/2 200 OK\r\n\r\n", "HTTP/1.1 099 x\r\n\r\n", "HTTP/1.1 600 x\r\n\r\n",
        "HTTP/1.1 200OK\r\n\r\n", "HTTP/1.1 200 OK\r\nBad\r\n\r\n",
        "HTTP/1.1 200 OK\r\nContent-Length: -1\r\n\r\n",
        "HTTP/1.1 200 OK\r\nContent-Length: 4\r\n\r\nabc",
        "HTTP/1.1 200 OK\r\nContent-Length: 1\r\n\r\nabc",
        "HTTP/1.1 200 OK\r\nContent-Length: 0\r\nContent-Length: 0\r\n\r\n",
        "HTTP/1.1 200 OK\r\nContent-Length: 0\r\nTransfer-Encoding: chunked\r\n\r\n",
        "HTTP/1.1 200 OK\r\nTransfer-Encoding: gzip\r\n\r\n",
        "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\nx\r\n",
        "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n2\r\na",
        "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n1\r\naXX",
        "HTTP/1.1 204 No Content\r\n\r\nx",
        "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n0\r\nContent-Length: 2\r\n\r\n",
        "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n0\r\n\r\nx"));
TEST(Socket, InvalidAndMove) {
  Socket empty;
  EXPECT_FALSE(empty.valid());
  EXPECT_THROW(empty.receive(), std::logic_error);
  EXPECT_THROW(empty.sendAll("x"), std::logic_error);
  EXPECT_THROW(empty.localPort(), std::logic_error);
  auto server = Socket::listenTcp("127.0.0.1", 0);
  auto port = server.localPort();
  Socket moved(std::move(server));
  EXPECT_FALSE(server.valid());
  EXPECT_EQ(moved.localPort(), port);
  Socket other;
  other = std::move(moved);
  EXPECT_FALSE(moved.valid());
  EXPECT_EQ(other.localPort(), port);
  Socket adopted(other.release());
  EXPECT_FALSE(other.valid());
  EXPECT_EQ(adopted.localPort(), port);
  adopted.close();
  adopted.close();
}
TEST(Socket, TcpBinaryPartialEOF) {
  auto server = Socket::listenTcp("127.0.0.1", 0);
  server.setTimeout(2s);
  std::string payload(100000, 'a');
  payload[20] = '\0';
  auto task = std::async(std::launch::async, [&] {
    auto peer = server.accept();
    peer.setTimeout(2s);
    auto data = peer.receiveExact(payload.size());
    peer.sendAll(data.substr(0, 100));
    peer.sendAll(data.substr(100));
    return data;
  });
  auto client = Socket::connectTcp("127.0.0.1", server.localPort(), 2s);
  client.sendAll(payload);
  EXPECT_EQ(client.receiveExact(payload.size()), payload);
  EXPECT_TRUE(client.receive().empty());
  EXPECT_THROW(client.receiveExact(1), std::runtime_error);
  EXPECT_EQ(task.get(), payload);
}
TEST(Socket, TcpTimeoutAndValidation) {
  auto server = Socket::listenTcp("127.0.0.1", 0);
  server.setTimeout(100ms);
  EXPECT_THROW(server.accept(), std::system_error);
  EXPECT_THROW(server.setTimeout(0ms), std::invalid_argument);
  EXPECT_THROW(server.receive(0), std::invalid_argument);
  EXPECT_THROW(Socket::connectTcp("", 80), std::invalid_argument);
  EXPECT_THROW(Socket::connectTcp("127.0.0.1", 1, 0ms), std::invalid_argument);
  EXPECT_THROW(Socket::listenTcp("127.0.0.1", 0, 0), std::invalid_argument);
  auto client = Socket::connectTcp("127.0.0.1", server.localPort());
  auto peer = server.accept();
  peer.setTimeout(50ms);
  EXPECT_THROW(peer.receive(), std::system_error);
}
TEST(Socket, RefusedConnect) {
  auto s = Socket::listenTcp("127.0.0.1", 0);
  auto port = s.localPort();
  s.close();
  EXPECT_THROW(Socket::connectTcp("127.0.0.1", port, 200ms), std::system_error);
}
TEST(Socket, UdpBinaryAndEmpty) {
  auto a = Socket::bindUdp("127.0.0.1", 0), b = Socket::bindUdp("127.0.0.1", 0);
  a.setTimeout(1s);
  b.setTimeout(1s);
  a.sendTo(std::string("a\0b", 3), "127.0.0.1", b.localPort());
  auto d = b.receiveFrom();
  EXPECT_EQ(d.data, std::string("a\0b", 3));
  EXPECT_EQ(d.host, "127.0.0.1");
  EXPECT_EQ(d.port, a.localPort());
  b.sendTo("", d.host, d.port);
  EXPECT_EQ(a.receiveFrom().data, "");
  EXPECT_THROW(a.sendTo(std::string(65508, 'x'), "127.0.0.1", b.localPort()), std::length_error);
  EXPECT_THROW(a.receiveFrom(0), std::invalid_argument);
  a.sendTo("abc", "127.0.0.1", b.localPort());
  EXPECT_THROW(b.receiveFrom(2), std::length_error);
}
TEST(Socket, Ipv6Loopback) {
  Socket server;
  try {
    server = Socket::listenTcp("::1", 0);
  } catch (const std::exception& e) {
    GTEST_SKIP() << e.what();
  }
  server.setTimeout(1s);
  auto client = Socket::connectTcp("::1", server.localPort(), 1s);
  auto peer = server.accept();
  peer.setTimeout(1s);
  client.sendAll("hi");
  EXPECT_EQ(peer.receiveExact(2), "hi");
}
static HttpResponse localHttp(const std::string& response, const std::string& method = "GET",
                              size_t max = 8 * 1024 * 1024) {
  auto server = Socket::listenTcp("127.0.0.1", 0);
  server.setTimeout(2s);
  auto task = std::async(std::launch::async, [&] {
    auto peer = server.accept();
    peer.setTimeout(2s);
    std::string req;
    while (req.find("\r\n\r\n") == req.npos) req += peer.receive();
    peer.sendAll(response);
    return req;
  });
  try {
    auto result = httpRequest("http://127.0.0.1:" + std::to_string(server.localPort()) + "/test",
                              method, {}, "", 2s, max);
    EXPECT_NE(task.get().find(method + " /test HTTP/1.1"), std::string::npos);
    return result;
  } catch (...) {
    task.get();
    throw;
  }
}
TEST(HTTPClient, Loopback) {
  EXPECT_EQ(localHttp("HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nok").body, "ok");
  EXPECT_EQ(
      localHttp("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n2\r\nok\r\n0\r\n\r\n").body,
      "ok");
}
TEST(HTTPClient, HeadAndLimits) {
  auto r = localHttp("HTTP/1.1 200 OK\r\nContent-Length: 999\r\n\r\n", "HEAD");
  EXPECT_EQ(r.body, "");
  EXPECT_EQ(r.headers["content-length"], "999");
  EXPECT_THROW(localHttp("HTTP/1.1 200 OK\r\n\r\nlongbody", "GET", 10), std::length_error);
  EXPECT_THROW(httpRequest("https://example.com"), std::invalid_argument);
}

TEST(HTTP, HeaderAndBodyLimits) {
  EXPECT_THROW(parseHttpResponse("HTTP/1.1 200 " + std::string(65536, 'x') + "\r\n\r\n"),
               std::length_error);
  EXPECT_THROW(parseHttpResponse("HTTP/1.1 200 OK\r\nX: " + std::string(65536, 'x') + "\r\n\r\n"),
               std::length_error);
  EXPECT_THROW(parseHttpResponse("HTTP/1.1 200 OK\r\nContent-Length: abc\r\n\r\n", 100, true),
               std::invalid_argument);
  EXPECT_THROW(
      parseHttpResponse(
          "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n4\r\nabcd\r\n0\r\n\r\n", 3),
      std::length_error);
}
TEST(IP, EmbeddedNullRejected) {
  EXPECT_FALSE(isIPv4(std::string("127.0.0.1\0evil", 14)));
  EXPECT_FALSE(isIPv6(std::string("::1\0evil", 8)));
  EXPECT_THROW(Socket::connectTcp(std::string("127.0.0.1\0evil", 14), 80), std::invalid_argument);
}
