#pragma once
#include <thread>
#include <utility>
namespace simpleutils {
class ThreadWrapper {
  std::thread thread_;

 public:
  template <class F, class... A>
  explicit ThreadWrapper(F&& f, A&&... a) : thread_(std::forward<F>(f), std::forward<A>(a)...) {}
  ThreadWrapper(const ThreadWrapper&) = delete;
  ThreadWrapper& operator=(const ThreadWrapper&) = delete;
  ThreadWrapper(ThreadWrapper&&) = default;
  ThreadWrapper& operator=(ThreadWrapper&&) = delete;
  ~ThreadWrapper() {
    if (thread_.joinable()) thread_.join();
  }
  void join() {
    if (thread_.joinable()) thread_.join();
  }
  bool joinable() const noexcept { return thread_.joinable(); }
};
}  // namespace simpleutils
