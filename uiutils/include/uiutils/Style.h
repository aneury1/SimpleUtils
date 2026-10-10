#pragma once
#include <uiutils/Types.h>
#include <map>
#include <variant>
#include <type_traits>
namespace uiutils {
class Element;
enum class StyleKey { Background, Foreground, FontFace, FontSize, Bold, Italic,
    Underline, MinSize, MaxSize, Tooltip, Enabled, Visible };
class WidgetStyle {
public:
    using Value = std::variant<Color, Size, std::string, int, bool>;
    template<StyleKey Key> using Type = std::conditional_t<
        Key == StyleKey::Background || Key == StyleKey::Foreground, Color,
        std::conditional_t<Key == StyleKey::MinSize || Key == StyleKey::MaxSize, Size,
        std::conditional_t<Key == StyleKey::FontFace || Key == StyleKey::Tooltip, std::string,
        std::conditional_t<Key == StyleKey::FontSize, int, bool>>>>;
    template<StyleKey Key, class T> WidgetStyle& Set(T&& value) {
        using Expected = Type<Key>;
        static_assert(std::is_same_v<std::decay_t<T>, Expected> ||
            (std::is_same_v<Expected, std::string> && std::is_convertible_v<T, std::string>),
            "Wrong value type for StyleKey");
        return Set(Key, Value{Expected(std::forward<T>(value))});
    }
    WidgetStyle& Set(StyleKey key, Value value) {
        bool valid = false;
        switch (key) {
        case StyleKey::Background: case StyleKey::Foreground: valid = std::holds_alternative<Color>(value); break;
        case StyleKey::MinSize: case StyleKey::MaxSize:
            if (auto* size = std::get_if<Size>(&value)) valid = size->width >= -1 && size->height >= -1;
            break;
        case StyleKey::FontFace: case StyleKey::Tooltip: valid = std::holds_alternative<std::string>(value); break;
        case StyleKey::FontSize: if (auto* v = std::get_if<int>(&value)) valid = *v > 0; break;
        case StyleKey::Bold: case StyleKey::Italic: case StyleKey::Underline:
        case StyleKey::Enabled: case StyleKey::Visible: valid = std::holds_alternative<bool>(value); break;
        default: throw std::invalid_argument("Unknown style key");
        }
        if (!valid) throw std::invalid_argument("Invalid style value");
        values_.insert_or_assign(key, std::move(value)); return *this;
    }
    bool Contains(StyleKey key) const { return values_.count(key) != 0; }
    WidgetStyle& Remove(StyleKey key) { values_.erase(key); return *this; }
    WidgetStyle& Merge(const WidgetStyle& overrides) {
        for (const auto& entry : overrides.values_) values_.insert_or_assign(entry.first, entry.second);
        return *this;
    }
    template<class T> const T* Get(StyleKey key) const {
        auto it = values_.find(key); return it == values_.end() ? nullptr : std::get_if<T>(&it->second);
    }
    void Apply(const Element& element) const;
private:
    std::map<StyleKey, Value> values_;
};
}
