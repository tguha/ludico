#pragma once

#include "util/types.hpp"

struct Bitset128 {
    template <typename T>
    inline void set(T i) {
        v |= (u128(1) << u128(i));
    }

    template <typename T>
    inline bool get(T i) const {
        return !!(v & (u128(1) << u128(i)));
    }

    template <typename T>
    inline bool operator[](T i) const {
        return get(i);
    }

    template <typename T>
    inline void clear(T i) {
        v &= ~(u128(1) << u128(i));
    }

    inline void reset() {
        v = 0;
    }

private:
    u128 v = 0;
};
