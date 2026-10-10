#pragma once
#include <exception>
#include <functional>
#include <map>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
namespace simpleutils {
template <class F>
class ScopeExit {
  F f;
  bool active = true;

 public:
  explicit ScopeExit(F fn) : f(std::move(fn)) {}
  ScopeExit(const ScopeExit&) = delete;
  ScopeExit& operator=(const ScopeExit&) = delete;
  ScopeExit(ScopeExit&& o) noexcept(std::is_nothrow_move_constructible_v<F>)
      : f(std::move(o.f)), active(o.active) {
    o.release();
  }
  ~ScopeExit() noexcept {
    if (active) f();
  }  // callbacks must not throw
  void release() noexcept { active = false; }
};
template <class F>
auto scopeExit(F f) {
  return ScopeExit<F>(std::move(f));
}
template <class F, class G>
auto compose(F f, G g) {
  return [f = std::move(f), g = std::move(g)](auto&&... args) mutable -> decltype(auto) {
    return std::invoke(f, std::invoke(g, std::forward<decltype(args)>(args)...));
  };
}
template <class F, class G>
auto pipe(F f, G g) {
  return compose(std::move(g), std::move(f));
}

// Retries only the selected exception type; attempts includes the first call.
// Callable side effects must be safe to repeat. No delay is imposed.
template <class Exception = std::exception, class F>
decltype(auto) retry(size_t attempts, F&& f) {
  if (attempts == 0) throw std::invalid_argument("Zero retry attempts");
  for (size_t attempt = 0;; ++attempt) {
    try {
      return std::invoke(f);
    } catch (const Exception&) {
      if (attempt + 1 == attempts) throw;
    }
  }
}
template <class... F>
struct Overloaded : F... {
  using F::operator()...;
};
template <class... F>
Overloaded(F...) -> Overloaded<F...>;
// Explicit signature memoization for copyable results and ordered argument tuples.
// Cache is per object and not internally synchronized; failures are not cached.
template <class Signature, class F>
class Memoized;
template <class R, class... A, class F>
class Memoized<R(A...), F> {
  static_assert(!std::is_void_v<R> && !std::is_reference_v<R>, "Memoized result must be a value");
  F fn_;
  std::map<std::tuple<std::decay_t<A>...>, R> cache_;

 public:
  explicit Memoized(F f) : fn_(std::move(f)) {}
  R operator()(A... a) {
    auto key = std::make_tuple(a...);
    auto it = cache_.find(key);
    if (it != cache_.end()) return it->second;
    auto result = std::invoke(fn_, std::forward<A>(a)...);
    cache_.emplace(std::move(key), result);
    return result;
  }
  void clear() { cache_.clear(); }
  size_t cacheSize() const noexcept { return cache_.size(); }
};
template <class Signature, class F>
auto memoize(F f) {
  return Memoized<Signature, F>(std::move(f));
}
}  // namespace simpleutils
