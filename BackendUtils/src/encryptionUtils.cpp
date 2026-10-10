#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <simpleutils/encryptionUtils.h>

#include <algorithm>
#include <climits>
#include <memory>
#include <stdexcept>
namespace simpleutils {
static int length(size_t n) {
  if (n > INT_MAX) throw std::length_error("OpenSSL input too large");
  return static_cast<int>(n);
}
static void check(int r) {
  if (r != 1) throw std::runtime_error("OpenSSL operation failed");
}
std::vector<uint8_t> randomBytes(size_t n) {
  int count = length(n);
  std::vector<uint8_t> r(n);
  if (n) check(RAND_bytes(r.data(), count));
  return r;
}
EncryptionKey generateEncryptionKey() {
  auto b = randomBytes(32);
  EncryptionKey k{};
  std::copy(b.begin(), b.end(), k.begin());
  OPENSSL_cleanse(b.data(), b.size());
  return k;
}
using Context = std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)>;
std::vector<uint8_t> encrypt(const std::vector<uint8_t>& p, const EncryptionKey& k,
                             std::string_view aad, EncryptionAlgorithm a) {
  if (a != EncryptionAlgorithm::AES256GCM) throw std::invalid_argument("Unknown algorithm");
  length(p.size());
  length(aad.size());
  auto nonce = randomBytes(12);
  std::vector<uint8_t> r(32 + p.size() + 16);
  std::copy_n("SU01", 4, r.begin());
  std::copy(nonce.begin(), nonce.end(), r.begin() + 4);
  Context ctx(EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
  if (!ctx) throw std::bad_alloc();
  check(EVP_EncryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr, k.data(), nonce.data()));
  int n = 0, total = 0;
  if (!aad.empty())
    check(EVP_EncryptUpdate(ctx.get(), nullptr, &n, reinterpret_cast<const uint8_t*>(aad.data()),
                            length(aad.size())));
  if (!p.empty()) {
    check(EVP_EncryptUpdate(ctx.get(), r.data() + 32, &n, p.data(), length(p.size())));
    total = n;
  }
  check(EVP_EncryptFinal_ex(ctx.get(), r.data() + 32 + total, &n));
  total += n;
  check(EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_GET_TAG, 16, r.data() + 16));
  r.resize(32 + total);
  return r;
}
std::vector<uint8_t> decrypt(const std::vector<uint8_t>& e, const EncryptionKey& k,
                             std::string_view aad, EncryptionAlgorithm a) {
  if (a != EncryptionAlgorithm::AES256GCM) throw std::invalid_argument("Unknown algorithm");
  if (e.size() < 32 || !std::equal(e.begin(), e.begin() + 4, "SU01"))
    throw std::invalid_argument("Invalid encryption envelope");
  length(e.size() - 32);
  length(aad.size());
  Context ctx(EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
  if (!ctx) throw std::bad_alloc();
  std::vector<uint8_t> p(e.size() - 32 + 16);
  int n = 0, total = 0;
  auto wipe = [&] { OPENSSL_cleanse(p.data(), p.size()); };
  try {
    check(EVP_DecryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr, k.data(), e.data() + 4));
    if (!aad.empty())
      check(EVP_DecryptUpdate(ctx.get(), nullptr, &n, reinterpret_cast<const uint8_t*>(aad.data()),
                              length(aad.size())));
    if (e.size() > 32) {
      check(EVP_DecryptUpdate(ctx.get(), p.data(), &n, e.data() + 32, length(e.size() - 32)));
      total = n;
    }
    check(EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_TAG, 16,
                              const_cast<uint8_t*>(e.data() + 16)));
    if (EVP_DecryptFinal_ex(ctx.get(), p.data() + total, &n) != 1)
      throw std::runtime_error("Authentication failed");
    total += n;
    p.resize(total);
    return p;
  } catch (...) {
    wipe();
    throw;
  }
}
std::string encryptText(std::string_view p, const EncryptionKey& k, std::string_view aad) {
  return toHex(encrypt(toBytes(p), k, aad));
}
std::string decryptText(std::string_view p, const EncryptionKey& k, std::string_view aad) {
  return toString(decrypt(fromHex(p), k, aad));
}
void encryptFile(const std::string& i, const std::string& o, const EncryptionKey& k,
                 std::string_view a) {
  WriteFile(o, toString(encrypt(ReadBinaryFile(i), k, a)));
}
void decryptFile(const std::string& i, const std::string& o, const EncryptionKey& k,
                 std::string_view a) {
  auto p = decrypt(ReadBinaryFile(i), k, a);
  try {
    WriteFile(o, toString(p));
  } catch (...) {
    OPENSSL_cleanse(p.data(), p.size());
    throw;
  }
  OPENSSL_cleanse(p.data(), p.size());
}
EncryptionKey deriveEncryptionKey(std::string_view p, const std::vector<uint8_t>& salt,
                                  int iterations) {
  if (p.empty() || salt.size() < 16 || iterations < 100000)
    throw std::invalid_argument("Password, 16-byte salt and >=100000 iterations required");
  EncryptionKey k{};
  check(PKCS5_PBKDF2_HMAC(p.data(), length(p.size()), salt.data(), length(salt.size()), iterations,
                          EVP_sha256(), 32, k.data()));
  return k;
}
}  // namespace simpleutils
