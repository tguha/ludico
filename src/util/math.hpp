#pragma once

#include "util/util.hpp"
#include "util/types.hpp"
#include "util/macros.hpp"

// CONSTANTS
namespace math {
// using math:: instead of math:: to access glm functions
using namespace glm;

static constexpr f32
    TAU = math::pi<f32>() * 2,
    PI = math::pi<f32>(),
    PI_2 = math::pi<f32>() / 2.0f,
    PI_4 = math::pi<f32>() / 4.0f,
    PI_8 = math::pi<f32>() / 8.0f,
    PI_16 = math::pi<f32>() / 8.0f;

static constexpr usize
    AXIS_X = 0,
    AXIS_Y = 1,
    AXIS_Z = 2;

template <typename F>
inline int ticked_float_to_int(u64 ticks, F f) {
    int i = static_cast<int>(math::floor(math::abs(f)));
    auto g = math::abs(f) - i;
    int j =
        math::abs(g) > 0.001
            && ((ticks % static_cast<int>(1.0f / math::abs(g))) == 0) ? 1 : 0;
    return static_cast<int>(math::sign(f) * (i + j));
}

// TEMPLATE UTILITIES
// vec
template <class>
struct is_vec : std::false_type {};

template <glm::length_t L, class T, glm::qualifier Q>
struct is_vec<vec<L, T, Q>> : std::true_type {};

template <typename T>
concept is_vec_c = is_vec<T>::value;

template <typename T>
inline constexpr auto is_vec_v = is_vec<T>::value;

// math::mat
template <class T>
struct is_mat {
    static const bool value = false;
};
template <glm::length_t C, glm::length_t R, class T, glm::qualifier Q>
struct is_mat<mat<C, R, T, Q>> {
    static const bool value = true;
};

template <class T>
struct vec_traits {};

template <glm::length_t _L, class _T, glm::qualifier _Q>
struct vec_traits<vec<_L, _T, _Q>> {
    static constexpr auto L = _L;
    using T = _T;
    static constexpr auto Q = _Q;
};

// util to convert (X, Z) vectors into (X, Y, Z) vectors
template <class T, glm::qualifier Q>
inline vec<3, T, Q> xz_to_xyz(const vec<2, T, Q> &v, T y) {
    return vec<3, T, Q>(v.x, y, v.y);
}

// util to convert (X, Y, Z) vectors into (X, n, Z) vectors
template <class T, glm::qualifier Q>
inline vec<3, T, Q> xyz_to_xnz(const vec<3, T, Q> &v, T n) {
    return vec<3, T, Q>(v.x, n, v.z);
}

// custom glm extensions
template <
    glm::length_t L,
    class T,
    glm::qualifier Q>
inline bool nonzero(const vec<L, T, Q> &v, T epsilon = glm::epsilon<T>()) {
    return
        glm::any(
            glm::greaterThan(
                glm::abs(v), vec<L, T, Q>(epsilon)));
}

// override GLM dot(x, y) to support integer types

// FLOATING POINT T
template <glm::length_t L, class T, glm::qualifier Q>
    requires std::numeric_limits<T>::is_iec559
inline T dot(const vec<L, T, Q> &a, const vec<L, T, Q> &b) {
    return glm::dot(a, b);
}

// INTEGER T
template <glm::length_t L, class T, glm::qualifier Q>
    requires (!std::numeric_limits<T>::is_iec559)
inline T dot(const vec<L, T, Q> &a, const vec<L, T, Q> &b) {
    return (T) glm::dot(vec<L, f64, Q>(a), vec<L, f64, Q>(b));
}

// product of vector elements
template <
    glm::length_t L,
    class T,
    glm::qualifier Q>
inline auto prod(const vec<L, T, Q> &v) {
    T res = 1;
    for (usize i = 0; i < L; i++) {
        res *= v[i];
    }
    return res;
}

template <
    glm::length_t L,
    class T,
    glm::qualifier Q>
inline int nonzero_axis(const vec<L, T, Q> &v) {
    if constexpr(L >= 1) {
        if (v[0] != T(0)) {
            return 0;
        }
    }

    if constexpr(L >= 2) {
        if (v[1] != T(0)) {
            return 1;
        }
    }

    if constexpr(L >= 3) {
        if (v[2] != T(0)) {
            return 2;
        }
    }

    if constexpr(L >= 4) {
        if (v[3] != T(0)) {
            return 3;
        }
    }

    return -1;
}

template <class V>
    requires is_vec<V>::value
inline V make_axis(int axis, decltype(V::x) value = 1) {
    auto res = V(0);
    res[axis] = value;
    return res;
}

template <class V>
    requires is_vec<V>::value
inline V from_axes(std::initializer_list<std::tuple<int, decltype(V::x)>> ts) {
    auto res = V(0);
    for (const auto &[axis, value] : ts) {
        res[axis] = value;
    }
    return res;
}

template <class V>
    requires is_vec<V>::value
inline V make_unit(int axis) {
    auto res = V(0);
    res[axis] = 1;
    return res;
}

inline mat4 lerp_matrix_rt(
    const mat4 &start_m, const mat4 &end_m, float a) {
    return glm::interpolate(start_m, end_m, a);
}

inline mat4 dir_to_rotation(
    const vec3 &dir, const vec3 &up = vec3(0, 1, 0)) {
    const auto
        a0 = glm::normalize(glm::cross(up, dir)),
        a1 = glm::normalize(glm::cross(dir, a0)),
        a2 = glm::normalize(dir);
    return mat4(
        vec4(a0, 0),
        vec4(a1, 0),
        vec4(a2, 0),
        vec4(vec3(0), 1));
}

// lerp between two matrices
// NOTE: ignores skew and perspective components!
// WILL ONLY LERP OVER ROT/TRANS/SCALE
// TODO: rotation is broken
inline mat4 lerp_matrix(
    const mat4 &start_m, const mat4 &end_m, float a) {
    glm::quat start_rot, end_rot, res_rot;

    vec3
        start_scale, end_scale, res_scale,
        start_trans, end_trans, res_trans,
        start_skew, end_skew; // , res_skew;
    vec4 start_persp, end_persp; // , res_persp;
    glm::decompose(
        start_m, start_scale, start_rot, start_trans, start_skew, start_persp);
    glm::decompose(
        end_m, end_scale, end_rot, end_trans, end_skew, end_persp);

    // returned rotation is incorrect, correct it!
    // stackoverflow.com/questions/17918033/glm-decompose-mat4-into-translation-and-rotation
    start_rot = glm::conjugate(start_rot);
    end_rot = glm::conjugate(end_rot);

    res_rot = glm::lerp(start_rot, end_rot, a);
    res_trans = glm::lerp(start_trans, end_trans, a);
    res_scale = glm::lerp(start_scale, end_scale, a);

    // reconstruct matrices with interpolated values
    return
        glm::scale(mat4(1.0), res_scale)
            // * glm::toMat4(res_rot)
            * glm::translate(mat4(1.0), res_trans);
}

// find the smallest possible t such that s + t * ds is an integer
inline vec3 intbound(vec3 s, vec3 ds) {
    vec3 res;
    UNROLL(3)
    for (usize i = 0; i < 3; i++) {
        res[i] =
            ds[i] > 0 ?
                (glm::ceil(s[i]) - s[i])
                : (s[i] - glm::floor(s[i]));
    }
    res /= abs(ds);
    return res;
}

inline mat3 normal_matrix(const mat4 &model) {
    return inverseTranspose(mat3(model));
}

// bump function e^(-(1/(1-x^2)))
template <typename F>
inline auto bump(F x) {
    return e<F>() * exp(-1.0f / (1.0f - pow(x, 2.0f)));
}

inline auto xz_angle_to_dir(f32 xz) {
    return vec3(math::sin(xz), 0.0f, math::cos(xz));
}

// sin wave in [-1, 1] with a period of period_seconds
f32 time_wave(f32 period_seconds, f32 offset = 0.0f);

// constexpr math functions
namespace cexp {
constexpr auto square(auto a) {
    return a * a;
}

// WARNING: integer exponents only
template <typename T>
constexpr T pow(T a, auto n) {
    return n == 0 ? 1 : square(pow(a, n / 2)) * (n % 2 == 0 ?  1 : a);
}

template <typename T>
    requires std::is_integral_v<T>
constexpr T sqrt(T x) {
    struct {
        constexpr T operator()(T x, T lo, T hi) {
            constexpr auto mid = ((lo + hi + 1) / 2);
            return lo == hi ?
                lo
                : (((x / mid) < mid) ?
                    helper(x, lo, mid - 1)
                    : helper(x, mid, hi));
        }
    } helper;
  return helper(x, 0, x / 2 + 1);
}

template <typename T>
    requires std::is_integral_v<T>
constexpr u64 popcnt(T x) {
    usize n = 0;
    while (x) {
        x &= x - 1;
        n++;
    }
    return n;
}
}
}

using math::vec;
using math::vec2;
using math::vec3;
using math::vec4;

using math::uvec2;
using math::uvec3;
using math::uvec4;

using math::ivec2;
using math::ivec3;
using math::ivec4;

using math::bvec2;
using math::bvec3;
using math::bvec4;

using math::mat;
using math::mat2;
using math::mat3;
using math::mat4;
