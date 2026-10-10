#pragma once
#include <uiutils/Types.h>
#include <array>
#include <map>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <sstream>
#include <iomanip>
namespace restaurant {
struct Product { std::string name; std::string category; int cents; };
inline const std::array<Product, 12> products = {{
    {"Burger", "Main dishes", 850}, {"Chicken rice", "Main dishes", 950},
    {"Pasta", "Main dishes", 1050}, {"Grilled fish", "Main dishes", 1450},
    {"House salad", "Sides", 550}, {"Fries", "Sides", 300},
    {"Soup", "Sides", 450}, {"Water", "Drinks", 150},
    {"Juice", "Drinks", 300}, {"Coffee", "Drinks", 200},
    {"Chocolate cake", "Desserts", 450}, {"Ice cream", "Desserts", 350}
}};
struct OrderLine { size_t product; int quantity; };
struct Order { std::vector<OrderLine> lines; std::string notes; };
inline std::string Money(std::int64_t cents) {
    if (cents < 0) throw std::invalid_argument("Negative price");
    std::ostringstream out;
    out << "EUR " << cents / 100 << '.' << std::setw(2) << std::setfill('0') << cents % 100;
    return out.str();
}
class OrderBook {
    std::map<int, Order> orders_;
    int table_ = 0;
public:
    void SelectTable(int index) {
        if (index < 0 || index > 6) throw std::out_of_range("Invalid table");
        table_ = index;
    }
    Order& Current() { return orders_[table_]; }
    std::string TableName() const { return table_ == 6 ? "Takeaway" : "Table " + std::to_string(table_ + 1); }
    int Add(size_t index) {
        if (index >= products.size()) throw std::out_of_range("Invalid product");
        auto& lines = Current().lines;
        auto it = std::find_if(lines.begin(), lines.end(), [index](const auto& line) { return line.product == index; });
        if (it == lines.end()) { lines.push_back({index, 1}); return static_cast<int>(lines.size() - 1); }
        if (it->quantity == 99) return uiutils::NoSelection;
        ++it->quantity; return static_cast<int>(it - lines.begin());
    }
    void SetQuantity(int index, int quantity) {
        if (quantity < 1 || quantity > 99) throw std::out_of_range("Invalid quantity");
        if (index < 0 || static_cast<size_t>(index) >= Current().lines.size()) throw std::out_of_range("Invalid line");
        Current().lines[index].quantity = quantity;
    }
    void Remove(int index) {
        auto& lines = Current().lines;
        if (index < 0 || static_cast<size_t>(index) >= lines.size()) throw std::out_of_range("Invalid line");
        lines.erase(lines.begin() + index);
    }
    std::int64_t Total() {
        std::int64_t total = 0;
        for (const auto& line : Current().lines) total += std::int64_t(products.at(line.product).cents) * line.quantity;
        return total;
    }
    void Clear() { Current() = {}; }
};
}
