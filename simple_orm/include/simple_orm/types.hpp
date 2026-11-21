#pragma once
#include <cstdint>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

namespace simple_orm {
using Blob = std::vector<std::uint8_t>;
using Value = std::variant<std::nullptr_t, std::int64_t, double, std::string, Blob>;
using Row = std::unordered_map<std::string, Value>;

class Error : public std::runtime_error {
public:
    explicit Error(const std::string& msg, int code = 0) : std::runtime_error(msg), code_(code) {}
    int code() const noexcept { return code_; }
private:
    int code_;
};

template<typename T> struct optional_traits { using type = T; static constexpr bool is_optional = false; };
template<typename T> struct optional_traits<std::optional<T>> { using type = T; static constexpr bool is_optional = true; };
template<typename T> struct remove_cvref { using type = std::remove_cv_t<std::remove_reference_t<T>>; };
template<typename T> using remove_cvref_t = typename remove_cvref<T>::type;

template<typename T> Value toValue(const T& v) {
    using U = remove_cvref_t<T>;
    if constexpr (optional_traits<U>::is_optional) {
        if (!v) return nullptr;
        return toValue(*v);
    } else if constexpr (std::is_same_v<U, std::nullptr_t>) {
        return nullptr;
    } else if constexpr (std::is_same_v<U, Blob>) {
        return v;
    } else if constexpr (std::is_same_v<U, std::string>) {
        return v;
    } else if constexpr (std::is_convertible_v<const U&, std::string_view>) {
        return std::string(std::string_view(v));
    } else if constexpr (std::is_same_v<U, bool>) {
        return std::int64_t(v);
    } else if constexpr (std::is_enum_v<U> || std::is_integral_v<U>) {
        return static_cast<std::int64_t>(v);
    } else if constexpr (std::is_floating_point_v<U>) {
        return static_cast<double>(v);
    } else {
        static_assert(!sizeof(T), "Unsupported value type; use supported scalar/optional/Blob");
    }
}

template<typename T> T fromValue(const Value& v) {
    using U = remove_cvref_t<T>;
    if constexpr (optional_traits<U>::is_optional) {
        using Inner = typename optional_traits<U>::type;
        if (std::holds_alternative<std::nullptr_t>(v)) return std::nullopt;
        return fromValue<Inner>(v);
    } else {
        if (std::holds_alternative<std::nullptr_t>(v)) throw Error("Cannot convert SQL NULL to non-optional C++ type");
        if constexpr (std::is_same_v<U, Blob>) {
            if (auto p = std::get_if<Blob>(&v)) return *p;
        } else if constexpr (std::is_same_v<U, std::string>) {
            if (auto p = std::get_if<std::string>(&v)) return *p;
        } else if constexpr (std::is_same_v<U, bool>) {
            if (auto p = std::get_if<std::int64_t>(&v)) return *p != 0;
        } else if constexpr (std::is_enum_v<U> || std::is_integral_v<U>) {
            if (auto p = std::get_if<std::int64_t>(&v)) return static_cast<U>(*p);
        } else if constexpr (std::is_floating_point_v<U>) {
            if (auto p = std::get_if<double>(&v)) return static_cast<U>(*p);
            if (auto p = std::get_if<std::int64_t>(&v)) return static_cast<U>(*p);
        } else {
            static_assert(!sizeof(U), "Unsupported field type");
        }
        throw Error("SQLite value type differs from C++ field type");
    }
}

enum class DataType { Integer, Real, Text, Blob, Any };
template<typename T> constexpr DataType dataType() {
    using U = typename optional_traits<remove_cvref_t<T>>::type;
    if constexpr (std::is_same_v<U, Blob>) return DataType::Blob;
    else if constexpr (std::is_same_v<U, std::string>) return DataType::Text;
    else if constexpr (std::is_integral_v<U> || std::is_enum_v<U>) return DataType::Integer;
    else if constexpr (std::is_floating_point_v<U>) return DataType::Real;
    else { static_assert(!sizeof(U), "Unsupported column type"); }
}
}
