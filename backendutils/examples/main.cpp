#include <simpleutils/simpleutils.h>

#include <iostream>
int main() {
  using namespace simpleutils;
  auto key = generateEncryptionKey();
  auto encoded = encryptText("Hello, SimpleUtils", key);
  std::cout << decryptText(encoded, key) << '\n';
  auto words = split(trim(" one,two,three "), ",");
  std::cout << join(words, " | ") << '\n';
  auto squared = mapToVector(std::vector<int>{1, 2, 3}, [](int n) { return n * n; });
  std::cout << squared.back() << '\n';
  auto url = parseHttpUrl("http://localhost:8080/status?" + buildQuery({{"name", "Aneury Perez"}}));
  std::cout << makeHttpRequest("GET", url);
}
