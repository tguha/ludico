#pragma once

#include "util/util.hpp"
#include "util/types.hpp"
#include "util/math.hpp"
#include "util/macros.hpp"

template <typename T, usize N>
struct AABBT;

template <typename T>
struct is_aabb : std::false_type {};

template <typename T, usize N>
struct is_aabb<AABBT<T, N>> : std::true_type {};

template <typename T, usize N>
struct AABBT {
    using V = vec<N, T, math::defaultp>;
    using VB = vec<N, bool, math::defaultp>;
    using VT = vec<N + 1, T, math::defaultp>;
    using M = mat<N + 1, N + 1, T, math::defaultp>;

    V min, max;

    constexpr AABBT() = default;
    constexpr AABBT(V _max) : min(0), max(_max) {}
    constexpr AABBT(V _min, V _max) : min(_min), max(_max) {}

    template <typename A, usize B>
    explicit AABBT(const AABBT<A, B> &other) {
        *this = AABBT(V(other.min), V(other.max));
    }

    AABBT(std::initializer_list<V> vs) {
        *this = compute(*vs.begin(), *vs.end());
    }

    AABBT(std::tuple<V, V> vs) {
        *this = compute(std::get<0>(vs), std::get<1>(vs));
    }

    static inline AABBT<T, N> compute(V a, V b) {
        return AABBT(math::min(a, b), math::max(a, b));
    }

    template <size_t E>
    static inline AABBT<T, N> merge(std::span<const AABBT<T, N>, E> boxes) {
        AABBT<T, N> result = boxes[0];
        for (const auto &box : boxes) {
            result.min = math::min(result.min, box.min);
            result.max = math::max(result.min, box.max);
        }
        return result;
    }

    template <typename ...Ts>
        requires (sizeof...(Ts) > 0
                  && is_aabb<typename std::common_type_t<Ts...>>::value)
    static inline auto merge(Ts ...boxes) {
        auto result = std::get<0>(std::make_tuple(boxes...));

        ([&](const auto &b) {
            result.min = math::min(result.min, b.min);
            result.max = math::max(result.max, b.max);
        }(boxes), ...);

        return result;
    }

    template <size_t E>
    static inline AABBT<T, N> compute(const std::span<V, E> &vs) {
        V min = vs[0], max = vs[0];
        for (const auto &v : vs) {
            min = math::min(min, v);
            max = math::max(max, v);
        }
        return AABBT(min, max);
    }

    static inline AABBT<T, N> unit() {
        return AABBT(V(1));
    }

    template <usize _ = N>
        requires (N == 2)
    inline std::array<V, 4> points() const {
        return {
            V(min),
            V(max),
            V(min.x, max.y),
            V(max.x, min.y)
        };
    }

    template <usize _ = N>
        requires (N == 3)
    inline std::array<V, 8> points() const {
        return {
            V(min.x, min.y, min.z),
            V(min.x, max.y, min.z),
            V(max.x, min.y, min.z),
            V(max.x, max.y, min.z),

            V(min.x, min.y, max.z),
            V(min.x, max.y, max.z),
            V(max.x, min.y, max.z),
            V(max.x, max.y, max.z),
        };
    }

    // TODO: fix for 2D AABBs
    inline AABBT<T, N> transform(const M &m) const {
        // recompute min/max because transformation could to whatever to
        // coordinates
        return compute(
            (m * VT(this->min, 1)).xyz(),
            (m * VT(this->max, 1)).xyz());
    }

    inline AABBT<T, N> transform(std::function<V(V)> f) const {
        return compute(f(this->min), f(this->max));
    }

    template <usize O, typename R = vec<O, T, math::defaultp>, typename F>
    inline AABBT<T, O> ntransform(F f) const {
        return AABBT<T, O>::compute(f(this->min), f(this->max));
    }

    inline AABBT<T, N> translate(const V &v) const {
        return AABBT<T, N>(this->min + v, this->max + v);
    }

    // scales both min and max by v
    inline AABBT<T, N> scale(const V &v) const {
        return AABBT<T, N>(this->min * v, this->max * v);
    }

    // scales both min and max by s
    inline AABBT<T, N> scale(const T &s) const {
        return scale(V(s));
    }

    // scales, keeping min in place
    inline AABBT<T, N> scale_min(const V &v) const {
        const auto d = this->max - this->min;
        return AABBT<T, N>(this->min, this->min + (d * v));
    }

    // scales, keeping min in place
    inline AABBT<T, N> scale_min(const T &s) const {
        return scale_min(V(s, s, s));
    }

    // scales, keeping center in place
    inline AABBT<T, N> scale_center(const V &v) const {
        const auto
            c = (this->min + this->max) / static_cast<T>(2),
            d = this->max - this->min,
            h = d / static_cast<T>(2),
            e = h * v;
        return AABBT<T, N>(c - e, c + e);
    }

    // scales, keeping center in place
    inline AABBT<T, N> scale_center(const T &s) const {
        return scale_center(V(s));
    }

    // expands AABB by v, keeping center in place
    inline AABBT<T, N> expand_center(const V &v) const {
        const auto h = v / V(2);
        return AABBT<T, N>(this->min - h, this->max + h);
    }

    inline AABBT<T, N> clamp(const V &min, const V &max) const {
        return AABBT<T, N>(
            math::clamp(this->min, min, max),
            math::clamp(this->max, min, max));
    }

    inline AABBT<T, N> clamp(const AABBT<T, N> &aabb) const {
        return AABBT<T, N>(
            math::clamp(this->min, aabb.min, aabb.max),
            math::clamp(this->max, aabb.min, aabb.max));
    }

    // center on the specified point
    // can optionally only center on some axes
    inline AABBT<T, N> center_on(
        const V &v, VB axes = VB(1)) const {
        const auto
            d = this->max - this->min,
            h = d / static_cast<T>(2);

        return AABBT<T, N>(
            math::mix(this->min, v - h, V(axes)),
            math::mix(this->max, v + h, V(axes)));
    }

    // calculate the center
    inline V center() const {
        return (this->min + this->max) / static_cast<T>(2);
    }

    // calculate the size
    inline V size() const {
        V mod(0);

        if constexpr (std::is_integral_v<T>) {
            mod = V(1);
        }

        return (this->max - this->min) + mod;
    }

    // volume of AABB
    inline T volume() const {
        return math::prod(this->size());
    }

    // calculates an AABB representing the overlap of this AABB and another
    inline AABBT<T, N> intersect(const AABBT<T, N> other) const {
        const auto &a = *this, &b = other;
        AABBT<T, N> res;

        UNROLL(N)
        for (usize i = 0; i < N; i++) {
            res.min[i] =
                a.min[i] < b.min[i] ?
                    b.min[i] : a.min[i];
            res.max[i] =
                a.max[i] > b.max[i] ?
                    b.max[i] : a.max[i];
        }

        return res;
    }

    // calculates the collision depth of this AABB into other
    inline V depth(const AABBT<T, N> &other) const {
        const auto &a = *this, &b = other;
        V res,
          c_a = a.center(),
          c_b = b.center();

        UNROLL(N)
        for (usize i = 0; i < N; i++) {
            res[i] =
                c_a[i] < c_b[i] ?
                    a.max[i] - b.min[i]
                    : b.max[i] - a.min[i];
        }

        return res;
    }

    // returns true if this AABB collides with the other
    inline bool collides(const AABBT<T, N> &other) const {
        UNROLL(N)
        for (usize i = 0; i < N; i++) {
            if (this->min[i] >= other.max[i]
                || this->max[i] <= other.min[i]) {
                return false;
            }
        }

        return true;
    }

    // returns true if this AABB contains the specified point
    inline bool contains(const V &point) const {
        UNROLL(N)
        for (usize i = 0; i < N; i++) {
            if (point[i] < this->min[i] || point[i] > this->max[i]) {
                return false;
            }
        }

        return true;
    }

    inline std::string to_string() const {
        return
            "AABB" + std::to_string(N) + "("
                + math::to_string(this->min) + ", "
                + math::to_string(this->max) + ")";
    }
};

// explicit specializations of AABBT
using AABB = AABBT<f32, 3>;
using AABB2 = AABBT<f32, 2>;
using AABBi = AABBT<int, 3>;
using AABB2i = AABBT<int, 2>;
using AABBu = AABBT<u32, 3>;
using AABB2u = AABBT<u32, 2>;
