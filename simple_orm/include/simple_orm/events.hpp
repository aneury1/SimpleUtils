#pragma once
#include "types.hpp"
#include <algorithm>
#include <vector>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

namespace simple_orm {
enum class EventKind { Insert, Update, Delete, TableCreated, TableDropped, SchemaChanged, Opened, Closed, Committed, RolledBack };
struct Event {
    EventKind kind = EventKind::Update;
    std::string database;
    std::string table;
    std::string column; // empty for table/row-level events
    std::int64_t rowId = 0;
    std::optional<Value> oldValue;
    std::optional<Value> newValue;
};
using EventCallback = std::function<void(const Event&)>;
namespace detail {
struct Listener { std::size_t id; std::string table; std::string column; EventCallback callback; };
struct EventBus {
    std::mutex mutex;
    std::size_t nextId = 1;
    std::vector<Listener> listeners;
};
}
class Subscription {
public:
    Subscription() = default;
    Subscription(std::weak_ptr<detail::EventBus> bus, std::size_t id) : bus_(std::move(bus)), id_(id) {}
    ~Subscription() { unsubscribe(); }
    Subscription(const Subscription&) = delete;
    Subscription& operator=(const Subscription&) = delete;
    Subscription(Subscription&& o) noexcept : bus_(std::move(o.bus_)), id_(o.id_) { o.id_ = 0; }
    Subscription& operator=(Subscription&& o) noexcept {
        if (this != &o) { unsubscribe(); bus_ = std::move(o.bus_); id_ = o.id_; o.id_ = 0; }
        return *this;
    }
    void unsubscribe() noexcept {
        if (id_ == 0) return;
        if (auto b = bus_.lock()) {
            std::lock_guard<std::mutex> lock(b->mutex);
            auto& x = b->listeners;
            x.erase(std::remove_if(x.begin(), x.end(), [id=id_](const auto& y){return y.id == id;}),x.end());
        }
        id_ = 0;
    }
    bool active() const noexcept { return id_ != 0 && !bus_.expired(); }
private:
    std::weak_ptr<detail::EventBus> bus_;
    std::size_t id_ = 0;
};
}
