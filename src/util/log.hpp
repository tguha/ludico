#pragma once

#include "util/util.hpp"
#include "util/types.hpp"
#include "util/string.hpp"

#ifndef LOG
#define LOG(s, ...)                                    \
    log::out(                                          \
        log::DEFAULT, __FILE__, __LINE__, __FUNCTION__,\
        log::format(std::string(s), ##__VA_ARGS__))
#endif

#ifndef ERROR
#define ERROR(s, ...)                                  \
    log::out(                                          \
        log::ERROR, __FILE__, __LINE__, __FUNCTION__,  \
        log::format(std::string(s), ##__VA_ARGS__))
#endif

#ifndef WARN
#define WARN(s, ...)                                   \
    log::out(                                          \
        log::WARN, __FILE__, __LINE__, __FUNCTION__,   \
        log::format(std::string(s), ##__VA_ARGS__))
#endif

namespace log {
    enum Level {
        NORMAL,
        WARN,
        ERROR,
        DEBUG,
        DEFAULT = DEBUG
    };

    void write(Level level, const std::string &s);

    void out(
        Level level,
        const std::string &file,
        usize line,
        const std::string &fn,
        const std::string &s);

    template <typename ...Ts>
    static inline std::string format(const std::string &s, Ts&& ...ts) {
        return fmt::vformat(
            std::string_view(s),
            fmt::make_format_args(std::forward<Ts>(ts)...));
    }
}
