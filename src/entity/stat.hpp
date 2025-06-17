#pragma once

#include "util/util.hpp"
#include "util/types.hpp"
#include "util/math.hpp"
#include "serialize/annotations.hpp"
#include "global.hpp"

template <typename T>
struct Stat {
    Stat() = default;
    Stat(T _v,
         T min = std::numeric_limits<T>::min(),
         T max = std::numeric_limits<T>::max())
        : _v(_v),
          _min(min),
          _max(max) {}
    Stat(const Stat &rhs) { *this = rhs; }
    Stat(Stat &&rhs) { *this = rhs; }

    inline Stat &operator=(Stat &&rhs) {
        return (*this = static_cast<const Stat&>(rhs));
    }

    inline auto operator*() const {
        return this->_v;
    }

    inline operator T() const {
        return this->_v;
    }

    inline Stat &operator=(const Stat &rhs) {
        const auto old = this->_v;
        std::memcpy(this, &rhs, sizeof(*this));

        if (math::abs(this->_v - old) > 0.0001f) {
            this->_last_change_ticks = global.time->ticks;
        }

        return *this;
    }

    inline Stat &set(T value, bool silent = false) {
        if (silent) {
            this->_v = value;
        } else {
            *this = value;
        }

        return *this;
    }

    inline Stat &operator=(T value) {
        value = math::clamp(value, this->_min, this->_max);

        if (math::abs(this->_v - value) > 0.0001f) {
            this->_last_change_ticks = global.time->ticks;
        }

        this->_v = value;
        return *this;
    }

    inline u64 last_change_ticks() const {
        return this->_last_change_ticks;
    }

    inline f32 normalized() const {
        if constexpr (std::is_integral_v<T>) {
            return this->_v / (this->_max - this->_min + 1);
        } else {
            const auto d = this->_max - this->_min;
            return d < math::epsilon<T>() ?
                T(0)
                : (this->_v / (this->_max - this->_min));
        }
    }

    inline T min() const { return this->_min; }
    inline T max() const { return this->_max; }

    // operators
    Stat operator+(const T &rhs) const { auto s = *this; s = s._v + rhs; return s; }
    Stat operator-(const T &rhs) const { auto s = *this; s = s._v - rhs; return s; }
    Stat operator*(const T &rhs) const { auto s = *this; s = s._v * rhs; return s; }
    Stat operator/(const T &rhs) const { auto s = *this; s = s._v / rhs; return s; }
    Stat operator-() const { auto s = *this; s._v *= T(-1); return s; }

    Stat& operator+=(const T &rhs) { return (*this = *this + rhs); }
    Stat& operator-=(const T &rhs) { return (*this = *this - rhs); }
    Stat& operator*=(const T &rhs) { return (*this = *this * rhs); }
    Stat& operator/=(const T &rhs) { return (*this = *this / rhs); }

    bool operator==(const Stat &rhs) const { return this->_v == rhs._v; }
    auto operator<=>(const Stat &rhs) const { return this->_v <=> rhs._v; }

    Stat& operator%(const T &rhs) const requires (!std::is_floating_point_v<T>) {
        auto s = *this; s._v = s._v % rhs; return *this;
    }

    Stat& operator%=(const T &rhs) const requires (!std::is_floating_point_v<T>) {
        return (*this = *this / rhs);
    }

    Stat& operator++() const requires (!std::is_floating_point_v<T>) {
        this->_v = this->_v + T(1);
        return *this;
    }

    Stat operator++(int) const requires (!std::is_floating_point_v<T>) {
        Stat s = *this;
        (*this)++;
        return s;
    }

    Stat& operator--() const requires (!std::is_floating_point_v<T>) {
        this->_v = this->_v - T(1);
        return *this;
    }

    Stat operator--(int) const requires (!std::is_floating_point_v<T>) {
        Stat s = *this;
        (*this)--;
        return s;
    }

private:
    T _v, _min, _max;
    u64 _last_change_ticks = std::numeric_limits<u64>::max();
};

ARCHIMEDES_REFLECT_TYPE_REGEX("Stat<.*>")
