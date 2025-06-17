#pragma once

#include "util/log.hpp"
#include "util/util.hpp"
#include "util/stacktrace.hpp"

template <typename ...Args>
inline std::string __assert_fmt(const std::string &fmt = "", Args&&... args) {
    if (fmt.length() > 0) {
        return fmt::vformat(
            std::string_view(fmt),
            fmt::make_format_args(std::forward<Args>(args)...));
    }

    return "";
}

#ifndef ASSERT
#define ASSERT(_e, ...) do {                                                   \
        if (!(_e)) {                                                           \
            const auto __msg = __assert_fmt(__VA_ARGS__);                      \
            ERROR(                                                             \
                "ASSERTION FAILED{}{}", __msg.length() > 0 ? " " : "", __msg); \
            log::write(log::ERROR, stacktrace::get());                         \
            std::exit(1);                                                      \
        }                                                                      \
    } while (0)
#endif

#define ENOIMPL() ASSERT(false, "NOT IMPLEMENTED")

#define ASSERT_STATIC(_e, _msg)                                                \
    do { ([]<bool _f = (_e)>() { static_assert(_f, _msg); })(); } while (0);
