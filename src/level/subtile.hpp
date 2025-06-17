#pragma once

#include "tile/tile.hpp"
#include "util/math.hpp"
#include "util/util.hpp"
#include "util/assert.hpp"

struct Level;

namespace subtile {
constexpr auto COUNT = 8;
constexpr auto SIZE = uvec3(2);

enum Enum {
    ST_LOWER_BOTTOM_LEFT = 0,
    ST_LOWER_TOP_LEFT,
    ST_LOWER_TOP_RIGHT,
    ST_LOWER_BOTTOM_RIGHT,
    ST_UPPER_BOTTOM_LEFT,
    ST_UPPER_TOP_LEFT,
    ST_UPPER_TOP_RIGHT,
    ST_UPPER_BOTTOM_RIGHT,
};

constexpr std::array<uvec3, 8> OFFSETS = {
    uvec3(0, 0, 0),
    uvec3(0, 0, 1),
    uvec3(1, 0, 1),
    uvec3(1, 0, 0),
    uvec3(0, 1, 0),
    uvec3(0, 1, 1),
    uvec3(1, 1, 1),
    uvec3(1, 1, 0),
};

inline auto to_offset(Enum s) {
    return OFFSETS[s];
}

inline auto from_offset(const uvec3 &v) {
    for (usize i = 0; i < 8; i++) {
        if (v == OFFSETS[i]) {
            return Enum(i);
        }
    }
    ASSERT(false, fmt::format("{} is not a valid subtile", v));
}

// subtile set helper, using tile based offset
void set(
    Level &level,
    const ivec3 &offset_tile,
    const ivec3 &offset_sub,
    const Tile &tile,
    bool value);

// subtile set helper, using tile based offset
// if explicitly_subtile, then this will ONLY return true if there is a subtile
// defined at the specified location, not just a normal tile which happens to
// take up the space of the subtile
/* std::tuple<const Tile*, bool> get( */
/*     Level &level, */
/*     const ivec3 &offset_tile, */
/*     const ivec3 &offset_sub, */
/*     bool explicitly_subtile = true); */
/* } */
}
