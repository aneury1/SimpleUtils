#pragma once
#include <gtest/gtest.h>
#include <simpleutils/simpleutils.h>

#include <filesystem>
#include <future>
namespace test_support {
struct TempDir {
  std::filesystem::path path =
      std::filesystem::temp_directory_path() /
      ("simpleutils-test-" + simpleutils::toHex(simpleutils::randomBytes(12)));
  TempDir() { std::filesystem::create_directory(path); }
  ~TempDir() {
    std::error_code ec;
    std::filesystem::remove_all(path, ec);
  }
  std::string file(const std::string& name) const { return (path / name).string(); }
};
}  // namespace test_support
