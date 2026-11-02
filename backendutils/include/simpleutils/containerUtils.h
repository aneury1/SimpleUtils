#pragma once
#include <algorithm>
#include <functional>
#include <map>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <vector>
namespace simpleutils {
template <class C, class T>
bool contains(const C& c, const T& v) {
  return std::find(c.begin(), c.end(), v) != c.end();
}
template <class C, class F>
auto mapToVector(const C& c, F f) {
  using R = std::decay_t<std::invoke_result_t<F, decltype(*c.begin())>>;
  std::vector<R> r;
  r.reserve(c.size());
  for (const auto& x : c) r.push_back(std::invoke(f, x));
  return r;
}
template <class C, class F>
auto filterToVector(const C& c, F f) {
  std::vector<typename C::value_type> r;
  for (const auto& x : c)
    if (std::invoke(f, x)) r.push_back(x);
  return r;
}
template <class C, class F>
auto findOptional(const C& c, F f) -> std::optional<typename C::value_type> {
  auto i = std::find_if(c.begin(), c.end(), f);
  if (i == c.end()) return std::nullopt;
  return *i;
}
template <class T>
std::vector<T> uniqueStable(const std::vector<T>& c) {
  std::vector<T> r;
  for (const auto& x : c)
    if (!contains(r, x)) r.push_back(x);
  return r;
}
template <class C, class F>
auto groupBy(const C& c, F key) {
  using K = std::decay_t<std::invoke_result_t<F, decltype(*c.begin())>>;
  std::map<K, std::vector<typename C::value_type>> r;
  for (const auto& x : c) r[std::invoke(key, x)].push_back(x);
  return r;
}
template <class T>
std::vector<std::vector<T>> chunk(const std::vector<T>& c, size_t n) {
  if (!n) throw std::invalid_argument("Zero chunk size");
  std::vector<std::vector<T>> r;
  for (size_t i = 0; i < c.size();) {
    size_t end = i + std::min(n, c.size() - i);
    r.emplace_back(c.begin() + i, c.begin() + end);
    i = end;
  }
  return r;
}
template <class A, class B>
auto zip(const A& a, const B& b) {
  std::vector<std::pair<typename A::value_type, typename B::value_type>> r;
  auto i = a.begin();
  auto j = b.begin();
  for (; i != a.end() && j != b.end(); ++i, ++j) r.emplace_back(*i, *j);
  return r;
}
template <class M>
auto keys(const M& m) {
  std::vector<typename M::key_type> r;
  for (const auto& p : m) r.push_back(p.first);
  return r;
}
template <class M>
auto values(const M& m) {
  std::vector<typename M::mapped_type> r;
  for (const auto& p : m) r.push_back(p.second);
  return r;
}
template <class M>
auto getOr(const M& m, const typename M::key_type& k, typename M::mapped_type fallback) {
  auto i = m.find(k);
  return i == m.end() ? std::move(fallback) : i->second;
}
}  // namespace simpleutils
