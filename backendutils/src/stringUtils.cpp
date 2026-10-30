#include <simpleutils/stringUtils.h>

#include <algorithm>
#include <fstream>
#include <iterator>
#include <stdexcept>
namespace simpleutils {
std::string toString(const std::vector<uint8_t>& b) { return {b.begin(), b.end()}; }
std::vector<uint8_t> toBytes(std::string_view s) { return {s.begin(), s.end()}; }
std::vector<uint8_t> ReadBinaryFile(const std::string& p) {
  std::ifstream f(p, std::ios::binary);
  if (!f) throw std::runtime_error("Cannot open file: " + p);
  std::vector<uint8_t> b((std::istreambuf_iterator<char>(f)), {});
  if (f.bad()) throw std::runtime_error("Cannot read file: " + p);
  return b;
}
std::string ReadFile(const std::string& p) { return toString(ReadBinaryFile(p)); }
void WriteFile(const std::string& p, std::string_view s) {
  std::ofstream f(p, std::ios::binary | std::ios::trunc);
  if (!f) throw std::runtime_error("Cannot open file: " + p);
  f.write(s.data(), static_cast<std::streamsize>(s.size()));
  if (!f) throw std::runtime_error("Cannot write file: " + p);
}
std::string ExtractPathOnly(const std::string& p) {
  auto n = p.find_last_of("/\\");
  return n == p.npos ? "" : p.substr(0, n == 0 ? 1 : n);
}
std::string ExtractFileOnly(const std::string& p) {
  auto n = p.find_last_of("/\\");
  return n == p.npos ? p : p.substr(n + 1);
}
static bool space(char c) {
  return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\v' || c == '\f';
}
std::string trim(std::string_view s) {
  while (!s.empty() && space(s.front())) s.remove_prefix(1);
  while (!s.empty() && space(s.back())) s.remove_suffix(1);
  return std::string(s);
}
std::string toLower(std::string_view s) {
  std::string r(s);
  for (char& c : r)
    if (c >= 'A' && c <= 'Z') c += 32;
  return r;
}
std::string toUpper(std::string_view s) {
  std::string r(s);
  for (char& c : r)
    if (c >= 'a' && c <= 'z') c -= 32;
  return r;
}
bool startsWith(std::string_view s, std::string_view p) { return s.substr(0, p.size()) == p; }
bool endsWith(std::string_view s, std::string_view p) {
  return s.size() >= p.size() && s.substr(s.size() - p.size()) == p;
}
std::string replaceAll(std::string_view s, std::string_view from, std::string_view to) {
  if (from.empty()) throw std::invalid_argument("Empty replacement pattern");
  std::string r;
  size_t pos = 0, n;
  while ((n = s.find(from, pos)) != s.npos) {
    r.append(s.substr(pos, n - pos));
    r.append(to);
    pos = n + from.size();
  }
  r.append(s.substr(pos));
  return r;
}
std::vector<std::string> split(std::string_view s, std::string_view d, bool keep) {
  if (d.empty()) throw std::invalid_argument("Empty delimiter");
  std::vector<std::string> r;
  size_t pos = 0;
  while (true) {
    auto n = s.find(d, pos);
    auto t = s.substr(pos, n == s.npos ? s.size() - pos : n - pos);
    if (keep || !t.empty()) r.emplace_back(t);
    if (n == s.npos) break;
    pos = n + d.size();
  }
  return r;
}
std::string join(const std::vector<std::string>& v, std::string_view d) {
  std::string r;
  for (size_t i = 0; i < v.size(); ++i) {
    if (i) r += d;
    r += v[i];
  }
  return r;
}
std::vector<std::vector<std::string>> parseCSVText(std::string_view s, char d) {
  if (d == '"' || d == '\r' || d == '\n') throw std::invalid_argument("Invalid CSV delimiter");
  std::vector<std::vector<std::string>> rows;
  if (s.empty()) return rows;
  std::vector<std::string> row;
  std::string field;
  bool quoted = false, closed = false, start = true;
  for (size_t i = 0; i < s.size(); ++i) {
    char c = s[i];
    if (quoted) {
      if (c == '"') {
        if (i + 1 < s.size() && s[i + 1] == '"') {
          field += '"';
          ++i;
        } else {
          quoted = false;
          closed = true;
        }
      } else
        field += c;
      continue;
    }
    if (c == '"') {
      if (!start) throw std::invalid_argument("Unexpected CSV quote");
      quoted = true;
      start = false;
    } else if (c == d) {
      row.push_back(field);
      field.clear();
      closed = false;
      start = true;
    } else if (c == '\r' || c == '\n') {
      if (c == '\r' && i + 1 < s.size() && s[i + 1] == '\n') ++i;
      row.push_back(field);
      rows.push_back(row);
      row.clear();
      field.clear();
      closed = false;
      start = true;
    } else {
      if (closed) throw std::invalid_argument("Data after CSV quote");
      field += c;
      start = false;
    }
  }
  if (quoted) throw std::invalid_argument("Unclosed CSV quote");
  if (!row.empty() || !field.empty() || closed || s.back() == d) {
    row.push_back(field);
    rows.push_back(row);
  }
  return rows;
}
std::vector<std::string> splitCSV(const std::string& s, char d) {
  if (s.empty()) return {""};
  auto r = parseCSVText(s, d);
  if (r.size() != 1) throw std::invalid_argument("Expected one CSV record");
  return r[0];
}
std::vector<std::vector<std::string>> parseCSV(const std::string& p) {
  return parseCSVText(ReadFile(p));
}
std::string toHex(const std::vector<uint8_t>& b, bool upper) {
  const char* h = upper ? "0123456789ABCDEF" : "0123456789abcdef";
  std::string r;
  for (auto x : b) {
    r += h[x >> 4];
    r += h[x & 15];
  }
  return r;
}
std::vector<uint8_t> fromHex(std::string_view s) {
  if (s.size() % 2) throw std::invalid_argument("Odd hex length");
  auto digit = [](char c) -> int {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    throw std::invalid_argument("Invalid hex digit");
  };
  std::vector<uint8_t> r;
  for (size_t i = 0; i < s.size(); i += 2)
    r.push_back(static_cast<uint8_t>((digit(s[i]) << 4) | digit(s[i + 1])));
  return r;
}
size_t find(const char* s, const char* n) {
  if (!s || !n) throw std::invalid_argument("Null text");
  return std::string_view(s).find(n);
}
}  // namespace simpleutils
