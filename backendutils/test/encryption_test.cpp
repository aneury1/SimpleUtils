#include <openssl/evp.h>

#include "test_support.h"
using namespace simpleutils;
class CryptoSize : public testing::TestWithParam<int> {};
TEST_P(CryptoSize, BinaryRoundtrip) {
  EncryptionKey k{};
  std::vector<uint8_t> plain(GetParam());
  for (size_t i = 0; i < plain.size(); ++i) plain[i] = i % 256;
  auto e = encrypt(plain, k, "metadata");
  EXPECT_EQ(e.size(), 32 + plain.size());
  EXPECT_EQ(decrypt(e, k, "metadata"), plain);
  EXPECT_NE(encrypt(plain, k, "metadata"), e);
}
INSTANTIATE_TEST_SUITE_P(Lengths, CryptoSize,
                         testing::Values(0, 1, 15, 16, 17, 255, 256, 4096, 65536));
class Tamper : public testing::TestWithParam<int> {};
TEST_P(Tamper, RejectEachEnvelopeByte) {
  EncryptionKey k{};
  auto e = encrypt(toBytes("abcdefgh"), k);
  e[GetParam()] ^= 1;
  EXPECT_ANY_THROW(decrypt(e, k));
}
INSTANTIATE_TEST_SUITE_P(Authentication, Tamper, testing::Range(0, 40));
TEST(Crypto, WrongKeyAndAAD) {
  EncryptionKey k{}, bad{};
  bad[0] = 1;
  auto e = encrypt(toBytes("secret"), k, "aad");
  EXPECT_THROW(decrypt(e, bad, "aad"), std::runtime_error);
  EXPECT_THROW(decrypt(e, k, "bad"), std::runtime_error);
  EXPECT_THROW(decrypt(e, k), std::runtime_error);
}
TEST(Crypto, TruncationAndUnknown) {
  EncryptionKey k{};
  auto e = encrypt(toBytes("a"), k);
  e.pop_back();
  EXPECT_THROW(decrypt(e, k), std::runtime_error);
  EXPECT_THROW(decrypt({}, k), std::invalid_argument);
  EXPECT_THROW(encrypt({}, k, {}, static_cast<EncryptionAlgorithm>(8)), std::invalid_argument);
  EXPECT_THROW(decrypt({}, k, {}, static_cast<EncryptionAlgorithm>(8)), std::invalid_argument);
}
TEST(Crypto, TextBinaryAndRandom) {
  auto k = generateEncryptionKey();
  EXPECT_NE(k, generateEncryptionKey());
  std::string s("hello\0world", 11);
  EXPECT_EQ(decryptText(encryptText(s, k), k), s);
  EXPECT_TRUE(randomBytes(0).empty());
  EXPECT_EQ(randomBytes(24).size(), 24u);
  EXPECT_THROW(decryptText("gg", k), std::invalid_argument);
}
TEST(Crypto, IndependentEVPInteroperability) {
  EncryptionKey k{};
  auto envelope = encrypt(toBytes("interop"), k);
  auto ctx = EVP_CIPHER_CTX_new();
  ASSERT_NE(ctx, nullptr);
  auto guard = scopeExit([&] { EVP_CIPHER_CTX_free(ctx); });
  ASSERT_EQ(EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, k.data(), envelope.data() + 4), 1);
  unsigned char out[64]{};
  int n = 0, total = 0;
  ASSERT_EQ(
      EVP_DecryptUpdate(ctx, out, &n, envelope.data() + 32, static_cast<int>(envelope.size() - 32)),
      1);
  total = n;
  ASSERT_EQ(EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, envelope.data() + 16), 1);
  ASSERT_EQ(EVP_DecryptFinal_ex(ctx, out + total, &n), 1);
  EXPECT_EQ(std::string(reinterpret_cast<char*>(out), total + n), "interop");
}
TEST(Crypto, Derivation) {
  std::vector<uint8_t> salt(16, 1);
  auto k = deriveEncryptionKey("password", salt, 100000);
  EXPECT_EQ(k, deriveEncryptionKey("password", salt, 100000));
  EXPECT_NE(k, deriveEncryptionKey("other", salt, 100000));
  salt[0] = 2;
  EXPECT_NE(k, deriveEncryptionKey("password", salt, 100000));
  EXPECT_THROW(deriveEncryptionKey("", salt), std::invalid_argument);
  EXPECT_THROW(deriveEncryptionKey("p", {}), std::invalid_argument);
  EXPECT_THROW(deriveEncryptionKey("p", salt, 1), std::invalid_argument);
}
TEST(Crypto, FilesAndFailedAuthenticationPreservesOutput) {
  test_support::TempDir d;
  auto key = generateEncryptionKey();
  WriteFile(d.file("plain"), std::string("a\0b", 3));
  encryptFile(d.file("plain"), d.file("enc"), key);
  decryptFile(d.file("enc"), d.file("out"), key);
  EXPECT_EQ(ReadFile(d.file("out")), ReadFile(d.file("plain")));
  auto bad = ReadBinaryFile(d.file("enc"));
  bad[16] ^= 1;
  WriteFile(d.file("enc"), toString(bad));
  WriteFile(d.file("out"), "keep");
  EXPECT_THROW(decryptFile(d.file("enc"), d.file("out"), key), std::runtime_error);
  EXPECT_EQ(ReadFile(d.file("out")), "keep");
  EXPECT_THROW(encryptFile(d.file("missing"), d.file("enc"), key), std::runtime_error);
}
