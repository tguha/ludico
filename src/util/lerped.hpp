#pragma once

#include "util/types.hpp"
#include "global.hpp"

// wrapper for T which automatically interpolates over time
template <typename T>
struct Lerped {
    Lerped(T speed = 1.0f)
        : speed(speed) {}

    inline Lerped &operator=(const T &rhs) {
        //if (this->last_ticks != global.time->ticks) {
            this->last = this->lerped();
            this->last_ticks = global.time->ticks;
        //}

        if constexpr (std::is_floating_point_v<T>) {
            this->last =
                (math::isnan(this->last) || math::isinf(this->last))
                    ? 0.0f : this->last;
        }

        this->target = rhs;
        return *this;
    }

    inline operator T() const {
        return this->lerped();
    }

    inline T operator*() const {
        return static_cast<T>(*this);
    }

    inline T raw() const {
        return this->target;
    }

    inline T lerped() const {
        const auto s = math::sign(this->target - this->last);
        const auto v =
            this->last + (T(global.time->ticks - this->last_ticks) * speed * s);
        const auto w =
            this->target > T(0) ?
                math::min(v, this->target)
                : math::max(v, this->target);
        return (w - this->target) < T(speed / 10.0f) ? this->target : w;
    }

private:
    T target = T(0), last = T(0), speed;
    usize last_ticks;
};
