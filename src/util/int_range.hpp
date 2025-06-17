#pragma once

#include "util/util.hpp"

// INCLUSIVE range [min, max]
template <typename T>
struct IntRange {
    T min, max;

    inline T count() const {
        return this->max - this->min + 1;
    }

    inline bool contains(T t) const {
        return t >= min && t <= max;
    }

    template <typename U>
    inline T operator[](U u) const {
        ASSERT(u >= 0 && u <= U(this->count()));
        return min + u;
    }

    std::string to_string() const {
        return fmt::format("IntRange<{}>({}, {})", typeid(T).name(), min, max);
    }

    template <typename ...Args>
    static constexpr IntRange<T> merge(Args &&...args) {
        return { std::min({ args.min... }), std::max({ args.max... }) };
    }
};
