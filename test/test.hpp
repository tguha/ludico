#pragma once

#include <string>
#include <iostream>
#include <algorithm>

#define FMT_HEADER_ONLY
#include <fmt/core.h>

template <typename ...Args>
static std::string _assert_fmt(
    const std::string &fmt = "",
    Args&&... args) {
    if (fmt.length() > 0) {
        return fmt::vformat(
            std::string_view(fmt),
            fmt::make_format_args(std::forward<Args>(args)...));
    }

    return "";
}

#define LOG_PREFIX()  \
    fmt::format(      \
        "[{}:{}][{}]",\
        __FILE__,     \
        __LINE__,     \
        __FUNCTION__)

#define LOG(_fmt, ...)                        \
    std::cout                                 \
        << LOG_PREFIX()                       \
        << " "                                \
        << (fmt::format(_fmt, ## __VA_ARGS__))\
        << std::endl;

#define WARN LOG
#define ERR LOG

#define ASSERT(_e, ...) do {                                                   \
        if (!(_e)) {                                                           \
            const auto __msg = _assert_fmt(__VA_ARGS__);                       \
            LOG(                                                               \
                "ASSERTION FAILED{}{}", __msg.length() > 0 ? " " : "", __msg); \
            std::exit(1);                                                      \
        }                                                                      \
    } while (0)
