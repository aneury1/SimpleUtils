#pragma once
#include <string>
namespace simpleutils {
namespace sslHelper {
constexpr int RSA_KEY_LENGTH = 4096;
// Self-signed development certificate, with DNS/IP SAN. Private key is unencrypted.
bool generateX509(const std::string& certPath, const std::string& keyPath, long daysValid,
                  const std::string& commonName = "localhost");
}  // namespace sslHelper
}  // namespace simpleutils
