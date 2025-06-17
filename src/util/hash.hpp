#pragma once

#include "util/math.hpp"
#include "util/util.hpp"

template <typename ...Ts>
u64 hash(Ts&& ...ts) {
    u64 seed = 0;
    ([&](auto &&t) {
        u64 hash = std::hash<std::decay_t<decltype(t)>>{}(t);
        hash += 0x9E3779B97F4A7C15 + (seed << 6) + (seed >> 2);
        seed ^= hash;
    }(ts), ...);
    return seed;
}
