#pragma once

#include "util/collections.hpp"
#include "util/util.hpp"
#include "util/types.hpp"
#include "util/math.hpp"
#include "util/assert.hpp"

namespace Direction {
enum Enum : u8 {
    SOUTH = 0,
    NORTH = 1,
    EAST = 2,
    WEST = 3,
    UP = 4,
    DOWN = 5,
    COUNT = 6,

    TOP = UP,
    BOTTOM = DOWN,
};

static inline const std::array<Enum, 6> ALL =
    { SOUTH, NORTH, EAST, WEST, TOP, BOTTOM };

static inline const std::array<Enum, 4> CARDINAL =
    { SOUTH, NORTH, EAST, WEST };

inline Enum opposite(Enum d) {
    static constexpr Enum LOOKUP[] = {
        NORTH, SOUTH, WEST, EAST, DOWN, UP
    };
    return LOOKUP[d];
}

inline usize axis(Enum d) {
    static constexpr u8 LOOKUP[] = {
        2, 2, 0, 0, 1, 1
    };
    return LOOKUP[d];
}

template <typename T, typename U>
inline auto from_axis(T axis, U sign) {
    static constexpr Enum LOOKUP[] = {
        EAST, WEST, UP, DOWN, SOUTH, NORTH
    };
    return LOOKUP[(axis * 2) + (math::sign(sign) < 0 ? 1 : 0)];
}

inline const char *to_string(Enum d) {
    static const char *LOOKUP[] = {
        "SOUTH",
        "NORTH",
        "WEST",
        "EAST",
        "TOP",
        "BOTTOM"
    };
    return LOOKUP[d];
}

inline ivec3 to_ivec3(Enum d) {
    static constexpr ivec3 LOOKUP[] = {
        ivec3( 0,  0,  1),
        ivec3( 0,  0, -1),
        ivec3( 1,  0,  0),
        ivec3(-1,  0,  0),
        ivec3( 0,  1,  0),
        ivec3( 0, -1,  0)
    };
    return LOOKUP[d];
}

inline Enum from_ivec3(const ivec3 &v) {
    if      (v == ivec3( 0,  0,  1)) { return SOUTH; }
    else if (v == ivec3( 0,  0, -1)) { return NORTH; }
    else if (v == ivec3( 1,  0,  0)) { return EAST; }
    else if (v == ivec3(-1,  0,  0)) { return WEST; }
    else if (v == ivec3( 0,  1,  0)) { return TOP; }
    else if (v == ivec3( 0, -1,  0)) { return BOTTOM; }
    else { return SOUTH; }
}

inline vec3 to_vec3(Enum d) {
    return ((vec3 []) {
        vec3( 0,  0,  1),
        vec3( 0,  0, -1),
        vec3( 1,  0,  0),
        vec3(-1,  0,  0),
        vec3( 0,  1,  0),
        vec3( 0, -1,  0)})[d];
}

static inline Enum from_vec3(const vec3 &v) {
    if (math::all(math::epsilonEqual(v, vec3( 0,  0,  1), 0.00001f))) {
        return SOUTH;
    } else if (math::all(math::epsilonEqual(v, vec3( 0,  0, -1), 0.00001f))) {
        return NORTH;
    } else if (math::all(math::epsilonEqual(v, vec3( 1,  0,  0), 0.00001f))) {
        return EAST;
    } else if (math::all(math::epsilonEqual(v, vec3(-1,  0,  0), 0.00001f))) {
        return WEST;
    } else if (math::all(glm::epsilonEqual(v, vec3( 0,  1,  0), 0.00001f))) {
        return TOP;
    } else if (glm::all(glm::epsilonEqual(v, vec3( 0, -1,  0), 0.00001f))) {
        return BOTTOM;
    }

    return SOUTH;
}

inline Enum nearest(const vec3 &v) {
    f32 m = math::abs(v[0]);
    usize i = 0;

    UNROLL(3)
    for (usize j = 0; j < 3; j++) {
        const auto a = math::abs(v[j]);
        if (a > m) {
            m = a;
            i = j;
        }
    }

    return from_ivec3(math::make_axis<ivec3>(i, math::sign(v[i])));
}

template <typename R, typename T>
    requires (
        (requires (T t) {
            typename std::remove_reference_t<T>::value_type;
            t.size();
        }))
R expand_directional_array(T &&ts) {
    using V = typename std::remove_reference_t<T>::value_type;
    const auto us =
        collections::copy_to<std::vector<V>>(
            std::forward<T>(ts));
    R arr;
    if (us.size() == 1) {
        std::fill(arr.begin(), arr.end(), us[0]);
    } else if (us.size() == 3) {
        arr = {
            us[0], us[0],
            us[1], us[1],
            us[2], us[2],
        };
    } else if (us.size() == 6) {
        std::copy(
            us.begin(),
            us.end(),
            arr.begin());
    } else {
        ASSERT(
            false,
            "illegal size for ts {} (must be one of 1, 3, or 6)",
            us.size());
    }
    return arr;
}

template <typename R, typename T>
    requires (
        !(requires (T t) {
            typename std::remove_reference_t<T>::value_type;
            t.size();
        }))
R expand_directional_array(const T &t) {
    return expand_directional_array<R>(std::initializer_list<T> { t });
}
}
