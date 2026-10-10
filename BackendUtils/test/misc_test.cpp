#include <openssl/pem.h>
#include <openssl/x509v3.h>

#include "test_support.h"
#ifndef _WIN32
#include <sys/stat.h>
#endif
using namespace simpleutils;
TEST(IP, AddressValidation) {
  EXPECT_TRUE(isIPv4("127.0.0.1"));
  EXPECT_TRUE(isIPv4("255.255.255.255"));
  EXPECT_FALSE(isIPv4("256.1.1.1"));
  EXPECT_FALSE(isIPv4("localhost"));
  EXPECT_TRUE(isIPv6("::1"));
  EXPECT_TRUE(isIPv6("2001:db8::1"));
  EXPECT_FALSE(isIPv6("127.0.0.1"));
  EXPECT_FALSE(isIPv6("::zz"));
}
TEST(IP, Interfaces) {
  std::vector<IPInfo> addresses;
  try {
    addresses = GetLocalIPs2();
  } catch (const std::system_error& e) {
    if (e.code().value() == EPERM || e.code().value() == EACCES) {
      GTEST_SKIP() << "Interface enumeration denied: " << e.what();
    }
    throw;
  }
  for (const auto& a : addresses) {
    EXPECT_FALSE(a.interfaceName.empty());
    EXPECT_TRUE(isIPv4(a.ipAddress) || isIPv6(a.ipAddress));
  }
  auto all = GetAllLocalIPs();
  EXPECT_EQ(uniqueStable(all), all);
}
TEST(Cmd, DetectionNoShell) {
  EXPECT_FALSE(IsInstalled(nullptr));
  EXPECT_FALSE(IsInstalled("simpleutils-nonexistent-command-123456"));
  EXPECT_FALSE(IsInstalled("sh; touch /tmp/should-not-exist"));
  auto terminal = GetTerminal();
  EXPECT_GE(static_cast<int>(terminal), 0);
  EXPECT_LE(static_cast<int>(terminal), 4);
  (void)getLatestClangFormat();
  EXPECT_THROW(formatWithClangFormat("int main(){}", "simpleutils-absent"), std::invalid_argument);
}
#ifndef _WIN32
TEST(Cmd, FormatterInvocation) {
  test_support::TempDir d;
  auto script = d.file("formatter with spaces");
  WriteFile(script, "#!/bin/sh\nprintf 'formatted' > \"$2\"\n");
  ASSERT_EQ(chmod(script.c_str(), 0700), 0);
  EXPECT_TRUE(IsInstalled(script.c_str()));
  EXPECT_EQ(formatWithClangFormat("input", script), "formatted");
  WriteFile(script, "#!/bin/sh\nexit 2\n");
  EXPECT_THROW(formatWithClangFormat("input", script), std::runtime_error);
}
#endif
TEST(Certificate, InvalidArguments) {
  EXPECT_FALSE(sslHelper::generateX509("x", "y", 0));
  EXPECT_FALSE(sslHelper::generateX509("x", "y", 1, ""));
  EXPECT_FALSE(sslHelper::generateX509("x", "x", 1));
  EXPECT_FALSE(sslHelper::generateX509("x", "y", 1, "bad\nname"));
}
TEST(Certificate, GeneratedKeyMatchesAndHasSAN) {
  test_support::TempDir d;
  auto c = d.file("cert.pem"), k = d.file("key.pem");
  ASSERT_TRUE(sslHelper::generateX509(c, k, 1, "localhost"));
  FILE* cf = fopen(c.c_str(), "rb");
  ASSERT_NE(cf, nullptr);
  auto cert = PEM_read_X509(cf, nullptr, nullptr, nullptr);
  fclose(cf);
  ASSERT_NE(cert, nullptr);
  auto guard = scopeExit([&] { X509_free(cert); });
  FILE* kf = fopen(k.c_str(), "rb");
  ASSERT_NE(kf, nullptr);
  auto key = PEM_read_PrivateKey(kf, nullptr, nullptr, nullptr);
  fclose(kf);
  ASSERT_NE(key, nullptr);
  auto keyGuard = scopeExit([&] { EVP_PKEY_free(key); });
  EXPECT_EQ(X509_check_private_key(cert, key), 1);
  EXPECT_EQ(X509_verify(cert, key), 1);
  EXPECT_EQ(X509_check_host(cert, "localhost", 9, 0, nullptr), 1);
  EXPECT_GT(X509_get_version(cert), 0);
#ifndef _WIN32
  struct stat st{};
  ASSERT_EQ(stat(k.c_str(), &st), 0);
  EXPECT_EQ(st.st_mode & 0777, 0600);
#endif
}
int odr_probe();
TEST(Build, MultipleTranslationUnits) { EXPECT_EQ(odr_probe(), 3); }
