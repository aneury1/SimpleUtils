#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/x509v3.h>
#include <simpleutils/ipUtils.h>
#include <simpleutils/sslHelper.h>

#include <climits>
#include <memory>
#ifndef _WIN32
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
namespace simpleutils {
namespace sslHelper {
bool generateX509(const std::string& certPath, const std::string& keyPath, long days,
                  const std::string& cn) {
  if (days <= 0 || days > LONG_MAX / 86400 || cn.empty() || cn.size() > 253 ||
      certPath == keyPath || cn.find_first_of("\r\n,\0", 0, 4) != cn.npos)
    return false;
  using Ctx = std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)>;
  Ctx ctx(EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr), EVP_PKEY_CTX_free);
  if (!ctx || EVP_PKEY_keygen_init(ctx.get()) != 1 ||
      EVP_PKEY_CTX_set_rsa_keygen_bits(ctx.get(), RSA_KEY_LENGTH) != 1)
    return false;
  EVP_PKEY* raw = nullptr;
  if (EVP_PKEY_keygen(ctx.get(), &raw) != 1) return false;
  std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> key(raw, EVP_PKEY_free);
  std::unique_ptr<X509, decltype(&X509_free)> cert(X509_new(), X509_free);
  if (!cert) return false;
  unsigned char serial[16];
  if (RAND_bytes(serial, sizeof(serial)) != 1) return false;
  serial[0] &= 0x7f;
  serial[0] |= 1;
  std::unique_ptr<BIGNUM, decltype(&BN_free)> bn(BN_bin2bn(serial, sizeof(serial), nullptr),
                                                 BN_free);
  if (!bn) return false;
  std::unique_ptr<ASN1_INTEGER, decltype(&ASN1_INTEGER_free)> number(
      BN_to_ASN1_INTEGER(bn.get(), nullptr), ASN1_INTEGER_free);
  if (!number || X509_set_version(cert.get(), 2) != 1 ||
      X509_set_serialNumber(cert.get(), number.get()) != 1 ||
      !X509_gmtime_adj(X509_getm_notBefore(cert.get()), 0) ||
      !X509_gmtime_adj(X509_getm_notAfter(cert.get()), days * 86400) ||
      X509_set_pubkey(cert.get(), key.get()) != 1)
    return false;
  auto name = X509_get_subject_name(cert.get());
  if (X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC,
                                 reinterpret_cast<const unsigned char*>(cn.c_str()), -1, -1,
                                 0) != 1 ||
      X509_set_issuer_name(cert.get(), name) != 1)
    return false;
  std::string san = (isIPv4(cn) || isIPv6(cn) ? "IP:" : "DNS:") + cn;
  X509V3_CTX extContext{};
  X509V3_set_ctx(&extContext, cert.get(), cert.get(), nullptr, nullptr, 0);
  std::unique_ptr<X509_EXTENSION, decltype(&X509_EXTENSION_free)> ext(
      X509V3_EXT_conf_nid(nullptr, &extContext, NID_subject_alt_name, san.data()),
      X509_EXTENSION_free);
  if (!ext || X509_add_ext(cert.get(), ext.get(), -1) != 1 ||
      X509_sign(cert.get(), key.get(), EVP_sha256()) <= 0)
    return false;
  using Bio = std::unique_ptr<BIO, decltype(&BIO_free)>;
#ifndef _WIN32
  int fd = ::open(keyPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
  if (fd < 0) return false;
  if (fchmod(fd, 0600)) {
    ::close(fd);
    return false;
  }
  Bio k(BIO_new_fd(fd, BIO_CLOSE), BIO_free);
  if (!k) {
    ::close(fd);
    return false;
  }
#else
  Bio k(BIO_new_file(keyPath.c_str(), "wb"), BIO_free);
#endif
  Bio c(BIO_new_file(certPath.c_str(), "wb"), BIO_free);
  if (!k || !c) return false;
  return PEM_write_bio_PrivateKey(k.get(), key.get(), nullptr, nullptr, 0, nullptr, nullptr) == 1 &&
         PEM_write_bio_X509(c.get(), cert.get()) == 1 && BIO_flush(k.get()) == 1 &&
         BIO_flush(c.get()) == 1;
}
}  // namespace sslHelper
}  // namespace simpleutils
