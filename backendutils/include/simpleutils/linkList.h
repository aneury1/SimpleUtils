#pragma once
#include <iostream>
#include <list>
#include <utility>
namespace simpleutils {
template <class D>
struct link_node {
  D value;
  link_node* prev = nullptr;
  link_node* next = nullptr;
  explicit link_node(D v) : value(std::move(v)) {}
};  // non-owning compatibility node
// Owning value-semantic replacement for the leaking original implementation.
template <class T>
class link_list {
  std::list<T> data_;

 public:
  link_list& insert(T v) {
    data_.push_front(std::move(v));
    return *this;
  }
  size_t size() const noexcept { return data_.size(); }
  auto begin() { return data_.begin(); }
  auto end() { return data_.end(); }
  auto begin() const { return data_.begin(); }
  auto end() const { return data_.end(); }
  void print(std::ostream& out = std::cout) const {
    for (const auto& x : data_) out << x << '\n';
  }
};
}  // namespace simpleutils
