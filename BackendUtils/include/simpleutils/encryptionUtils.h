#pragma once
#include <array>

#include "stringUtils.h"
namespace simpleutils {
enum class EncryptionAlgorithm { AES256GCM };
using EncryptionKey = std::array<uint8_t, 32>;
EncryptionKey generateEncryptionKey();
std::vector<uint8_t> randomBytes(size_t size);
// Envelope: "SU01" (4), random nonce (12), GCM tag (16), ciphertext.
std::vector<uint8_t> encrypt(const std::vector<uint8_t>& plain, const EncryptionKey& key,
                             std::string_view aad = {},
                             EncryptionAlgorithm algorithm = EncryptionAlgorithm::AES256GCM);
std::vector<uint8_t> decrypt(const std::vector<uint8_t>& envelope, const EncryptionKey& key,
                             std::string_view aad = {},
                             EncryptionAlgorithm algorithm = EncryptionAlgorithm::AES256GCM);
std::string encryptText(std::string_view plain, const EncryptionKey& key,
                        std::string_view aad = {});  // hex envelope
std::string decryptText(std::string_view hex, const EncryptionKey& key, std::string_view aad = {});
void encryptFile(const std::string& input, const std::string& output, const EncryptionKey& key,
                 std::string_view aad = {});
void decryptFile(const std::string& input, const std::string& output, const EncryptionKey& key,
                 std::string_view aad = {});
EncryptionKey deriveEncryptionKey(std::string_view password, const std::vector<uint8_t>& salt,
                                  int iterations = 600000);
}  // namespace simpleutils
