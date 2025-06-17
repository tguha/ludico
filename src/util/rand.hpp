#pragma once

#include "util/math.hpp"
#include "util/types.hpp"
#include "util/util.hpp"
#include "util/assert.hpp"

// xorshift random number generator, supporting random integers up to
// sizeof(u64) and random floats up to sizeof(f64). 128-bit state.
struct Rand {
    Rand() = default;

    explicit Rand(std::optional<usize> seed) {
        if (seed) {
            this->seed(*seed);
        }
    }

    template <typename T>
    inline T next_normal(T mean, T stddev = T(1)) {
        // implement UniformRandomBitGenerator
        struct {
            Rand &rand;

            using result_type = usize;

            static constexpr result_type max() {
                return std::numeric_limits<usize>::max();
            }

            static constexpr result_type min() {
                return std::numeric_limits<usize>::min();
            }

            result_type operator()() {
                return rand.next<usize>();
            }
        } gen = { *this };

        std::normal_distribution<T> dist { mean, stddev };
        return dist(gen);
    }

    template <typename Container>
    inline auto pick(const Container &c) {
        return c[this->next<usize>(0, c.size() - 1)];
    }

    template <typename F>
    inline void n_times(usize min, usize max, F &&f) {
        for (usize i = this->next(min, max); i > 0; i--) {
            f();
        }
    }

    // random integer
    // NOTE: bounds are inclusive
    template <typename T>
        requires std::is_integral_v<T>
    inline T next(
        T min = std::numeric_limits<T>::min(),
        T max = std::numeric_limits<T>::max()) {
        ASSERT(max >= min, "max < min!");
        using R = decltype(xorshf64());
        static_assert(sizeof(T) <= sizeof(R));

        if constexpr (std::is_same_v<T, R>) {
            if (max == std::numeric_limits<R>::max()) {
                // TODO: overflow?
                return xorshf64() - (max - min + 1) + min;
            }
        }

        const auto divisor = R(max) - R(min) + R(1);
        const auto res = divisor == 0 ? xorshf64() : (xorshf64() % divisor);
        return T(res) + min;
    }

    // random floating point
    // NOTE: bounds are inclusive
    template <typename T>
        requires std::is_floating_point_v<T>
    inline T next(
        T min = std::numeric_limits<T>::min(),
        T max = std::numeric_limits<T>::max()) {
        const u64 r = next<u64>(0, std::numeric_limits<u64>::max());
        const union { u64 i; f64 d; } u =
            { .i = (u64(0x3FF) << 52) | (r >> 12) };

        // q is now a double in the unit interval
        const f64 q = u.d - 1.0;
        return (q * (max - min)) + min;
    }

    // random vector
    // NOTE: bounds are inclusive
    template <
        typename V,
        typename T = typename math::vec_traits<V>::T,
        usize L = math::vec_traits<V>::L>
        requires math::is_vec<V>::value
    inline V next(
        V min = V(std::numeric_limits<T>::min()),
        V max = V(std::numeric_limits<T>::max())) {
        V res;
        for (usize i = 0; i < L; i++) {
            res[i] = this->next<T>(min[i], max[i]);
        }
        return res;
    }

    template <typename T>
        requires std::is_floating_point_v<T>
    inline bool chance(T t) {
        return this->next<T>(T(0.0f), T(1.0f)) < t;
    }

    inline void seed(usize s) {
        this->s[0] *= s;
        this->s[1] *= s;
        this->s[2] *= s;
        this->s[3] *= s;
        xorshf32();
        xorshf32();
        xorshf32();
    }

private:
    inline u64 xorshf64() {
        return (u64(xorshf32()) << 32) | u64(xorshf32());
    }

    u32 s[4] = { 124528375, 2389572938, 1359875, 842598743 };

    inline u32 xorshf32() {
        // xorshift 128-bit state
        u32 t = s[3];
        u32 q = s[0];
        s[3] = s[2];
        s[2] = s[1];
        s[1] = q;
        t ^= t << 11;
        t ^= t >> 8;
        return s[0] = t ^ q ^ (q >> 19);
    }
};
