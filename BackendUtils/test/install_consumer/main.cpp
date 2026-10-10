#include <simpleutils/simpleutils.h>
#include <stringUtils.h>
int other();
int main() {
  auto key = simpleutils::generateEncryptionKey();
  auto encrypted = simpleutils::encryptText("installed", key);
  return simpleutils::decryptText(encrypted, key) == "installed" && other() == 2 ? 0 : 1;
}
