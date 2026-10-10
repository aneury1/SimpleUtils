#pragma once
#include <string>
#include <utility>
#include <vector>
#include <stdexcept>
#include <cctype>
namespace uiutils {
inline constexpr int NoSelection = -1;
inline constexpr int AutoId = -1;
struct Size { int width = -1; int height = -1; Size(int w = -1, int h = -1) : width(w), height(h) {} };
struct Color {
    std::string hex;
    explicit Color(std::string value) : hex(std::move(value)) {
        if (hex.size() != 7 || hex[0] != '#') throw std::invalid_argument("Use #RRGGBB colors");
        for (size_t i = 1; i < hex.size(); ++i)
            if (!std::isxdigit(static_cast<unsigned char>(hex[i]))) throw std::invalid_argument("Invalid color");
    }
};
using Items = std::vector<std::string>;
enum class Alignment { Start, Center, End };
enum class TextInputMode { SingleLine, MultiLine, Password, ReadOnly };
class Modifier {
public:
    int weight = 0, margin = 0;
    bool expand = false;
    Alignment alignment = Alignment::Start;
    Modifier Weight(int n = 1) const {
        if (n < 0) throw std::invalid_argument("Negative weight");
        auto m = *this; m.weight = n; return m;
    }
    Modifier Expand() const { auto m = *this; m.expand = true; return m; }
    Modifier Margin(int dip) const {
        if (dip < 0) throw std::invalid_argument("Negative margin");
        auto m = *this; m.margin = dip; return m;
    }
    Modifier Align(Alignment a) const { auto m = *this; m.alignment = a; return m; }
};
}
