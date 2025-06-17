#pragma once

#include "util/util.hpp"
#include "util/assert.hpp"

// try std::optional _o, otherwise return std::nullopt
#define TRY_OPT(_o) ({                                                         \
        decltype(_o) __o = (_o);                                               \
        if (!(__o)) {                                                          \
            return std::nullopt;                                               \
        }                                                                      \
        (*__o);                                                                \
    })

#define _OPT_OR_RET1(_o) ({                                                    \
        decltype(_o) __o = (_o);                                               \
        if (!(__o)) {                                                          \
            return;                                                            \
        }                                                                      \
        *(__o);                                                                \
    })

#define _OPT_OR_RET2(_o, _v) ({                                                \
        decltype(_o) __o = (_o);                                               \
        if (!(__o)) {                                                          \
            return (_v);                                                       \
        }                                                                      \
        *(__o);                                                                \
    })

// try std::optional [arg 0] otherwise return [arg 1] (if present)
#define OPT_OR_RET(...) VFUNC(_OPT_OR_RET, __VA_ARGS__)

// try std::optional, otherwise continue
#define OPT_OR_CONT(_o) ({                                                     \
        decltype(_o) __o = (_o);                                               \
        if (!(__o)) {                                                          \
            continue;                                                          \
        }                                                                      \
        *(__o);                                                                \
    })

// try std::optional, otherwise assert false
#define OPT_OR_ASSERT(_o, ...) ({                                              \
        decltype(_o) __o = (_o);                                               \
        ASSERT(__o, __VA_ARGS__);                                              \
        *(__o);                                                                \
    })

namespace util {
// map(opt, function) for std::optional
template <typename T, typename F>
    requires (!std::is_same_v<std::invoke_result_t<F, const T&>, void>)
inline auto map(const std::optional<T> &opt, F &&f) {
    return opt ? std::make_optional(f(*opt)) : std::nullopt;
}

template <typename T, typename F>
    requires (!std::is_same_v<std::invoke_result_t<F, T&>, void>)
inline auto map(std::optional<T> &opt, F &&f) {
    return opt ? std::make_optional(f(*opt)) : std::nullopt;
}


template <typename T, typename F>
    requires (std::is_same_v<std::invoke_result_t<F, const T&>, void>)
inline auto map(const std::optional<T> &opt, F &&f) {
    if (opt) { f(*opt); }
}

template <typename T, typename F>
    requires (std::is_same_v<std::invoke_result_t<F, T&>, void>)
inline auto map(std::optional<T> &opt, F &&f) {
    if (opt) { f(*opt); }
}
}
