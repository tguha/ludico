#pragma once

#ifndef IN_UTIL_HPP
#warning "do not include me directly! use util.hpp"
#endif

#include "util/glm.hpp"

#include <optional>
#include <vector>
#include <set>
#include <unordered_set>
#include <deque>
#include <span>
#include <array>
#include <initializer_list>

#define FMT_HEADER_ONLY
#include <fmt/core.h>
#include <fmt/format.h>

#include <nameof.hpp>

// custom formatters
namespace fmt {
template <typename>
struct is_vec : std::false_type {};

template <glm::length_t L, class T, glm::qualifier Q>
struct is_vec<glm::vec<L, T, Q>> : std::true_type {};

template <typename>
struct is_mat : std::false_type {};

template <glm::length_t C, glm::length_t R, class T, glm::qualifier Q>
struct is_mat<glm::mat<C, R, T, Q>> : std::true_type {};

template<class T>
struct is_vector : std::false_type {};

template<class T>
struct is_vector<std::vector<T>> : std::true_type {};

template<class T>
struct is_set : std::false_type {};

template<class T>
struct is_set<std::set<T>> : std::true_type {};

template<class T>
struct is_unordered_set : std::false_type {};

template<class T>
struct is_unordered_set<std::unordered_set<T>> : std::true_type {};

template<class T>
struct is_deque : std::false_type {};

template<class T>
struct is_deque<std::deque<T>> : std::true_type {};

template<typename T>
struct is_span : std::false_type {};

template<typename T, size_t E>
struct is_span<std::span<T, E>> : std::true_type {};

template<typename T>
struct is_array : std::false_type {};

template<typename T, size_t E>
struct is_array<std::array<T, E>> : std::true_type {};

template<typename T>
struct is_initializer_list : std::false_type {};

template<typename T>
struct is_initializer_list<std::initializer_list<T>> : std::true_type {};

// std::optional values
template <typename T>
struct formatter<std::optional<T>> : fmt::formatter<T> {
    template <typename FormatContext>
    auto format(const std::optional<T>& opt, FormatContext& ctx) const {
        if (opt) {
            fmt::formatter<T>::format(*opt, ctx);
            return ctx.out();
        }
        return fmt::format_to(ctx.out(), "(NO VALUE)");
    }
};

// std::unique_ptr
template <typename T>
struct formatter<std::unique_ptr<T>> : fmt::formatter<T> {
    template <typename FormatContext>
    auto format(const std::unique_ptr<T>& ptr, FormatContext& ctx) const {
        if (ptr) {
            fmt::formatter<T>::format(*ptr, ctx);
            return ctx.out();
        }
        return fmt::format_to(ctx.out(), "(nullptr)");
    }
};

// _.to_string() able
template <typename T>
    requires (
        requires (const T &t) {
            { t.to_string() } -> std::convertible_to<std::string>;
        })
struct formatter<T> : formatter<std::string> {
    template <typename FormatContext>
    auto format(const T& t, FormatContext& ctx) const {
        return formatter<std::string>::format(t.to_string(), ctx);
    }
};

// math types
template <typename T>
    requires (is_mat<T>::value || is_vec<T>::value)
struct formatter<T> : formatter<std::string> {
    template <typename FormatContext>
    auto format(const T& t, FormatContext& ctx) const {
        return formatter<std::string>::format(glm::to_string(t), ctx);
    }
};

// stdlib containers
template <typename T>
    requires (
        is_vector<T>::value
        || is_set<T>::value
        || is_unordered_set<T>::value
        || is_deque<T>::value
        || is_span<T>::value
        || is_array<T>::value)
struct formatter<T> : formatter<typename T::value_type> {
    template <typename FormatContext>
    auto format(const T& ts, FormatContext& ctx) const {
        return format_to(ctx.out(), "[{}]", fmt::join(ts, ", "));
    }
};

// enums
template <typename T>
    requires (std::is_enum_v<T>)
struct formatter<T> : formatter<std::string> {
    template <typename FormatContext>
    auto format(const T &t, FormatContext &ctx) const {
        return formatter<std::string>::format(std::string(NAMEOF_ENUM(t)), ctx);
    }
};
}
