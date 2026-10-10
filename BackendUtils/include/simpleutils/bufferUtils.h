#pragma once
#include <algorithm>
#include <stdexcept>

#include "stringUtils.h"
namespace simpleutils {
struct Buffer {
  std::vector<uint8_t> data;
  std::vector<uint8_t> pad(size_t total, uint8_t byte, bool left = false) {
    if (total > data.size())
      data.insert(left ? data.begin() : data.end(), total - data.size(), byte);
    return data;
  }
  void setData(const uint8_t* p, uint32_t n) {
    if (!p && n) throw std::invalid_argument("Null buffer");
    std::vector<uint8_t> tmp;
    if (n) tmp.assign(p, p + n);
    data = std::move(tmp);
  }
  void setData(const char* p, uint32_t n) { setData(reinterpret_cast<const uint8_t*>(p), n); }
  void setData(std::vector<uint8_t> b) { data = std::move(b); }
  void Mid(std::vector<uint8_t>& r, int first, int last) const {
    range(first, last);
    std::vector<uint8_t> tmp(data.begin() + first, data.begin() + last);
    r.insert(r.end(), tmp.begin(), tmp.end());
  }
  void Mid(std::string& r, int first, int last) const {
    range(first, last);
    r.append(data.begin() + first, data.begin() + last);
  }
  void insertInto(std::vector<uint8_t> b, int at) {
    range(at, at);
    data.insert(data.begin() + at, b.begin(), b.end());
  }
  void insertInto(Buffer b, int at) { insertInto(std::move(b.data), at); }
  void insertInto(std::string b, int at) { insertInto(toBytes(b), at); }
  void insertInto(const char* b, int at) {
    if (!b) throw std::invalid_argument("Null buffer");
    insertInto(std::string(b), at);
  }
  void append(uint8_t b) { data.push_back(b); }
  uint8_t& operator[](size_t n) { return data.at(n); }
  const uint8_t& operator[](size_t n) const { return data.at(n); }
  void removeFrom(int at, int count) {
    if (count < 0) throw std::invalid_argument("Negative count");
    range(at, at);
    auto n = std::min<size_t>(static_cast<size_t>(count), data.size() - at);
    data.erase(data.begin() + at, data.begin() + at + n);
  }
  using WorkerFunction = void (*)(Buffer*, unsigned char, int);
  void iterateOver(WorkerFunction f) {
    if (!f) throw std::invalid_argument("Null callback");
    auto copy = data;
    for (size_t i = 0; i < copy.size(); ++i) f(this, copy[i], static_cast<int>(i));
  }
  size_t size() const noexcept { return data.size(); }

 private:
  void range(int a, int b) const {
    if (a < 0 || b < a || static_cast<size_t>(b) > data.size())
      throw std::out_of_range("Buffer range");
  }
};
struct ByteUtils {
  static std::string toHexString(const Buffer& b) { return toHex(b.data); }
  static Buffer toBuffer(uint16_t b) {
    return Buffer{{static_cast<uint8_t>(b >> 8), static_cast<uint8_t>(b)}};
  }
  static uint8_t reverseByte(uint8_t b) {
    uint8_t r = 0;
    for (int i = 0; i < 8; ++i) {
      r = static_cast<uint8_t>((r << 1) | (b & 1));
      b >>= 1;
    }
    return r;
  }
};
}  // namespace simpleutils
