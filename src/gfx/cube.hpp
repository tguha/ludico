#pragma once

#include "util/types.hpp"
#include "util/util.hpp"
#include "util/math.hpp"

namespace cube {
/* 3D CUBE (looking down +z)
 * 5-------6
 * |1------+2
 * ||      ||
 * ||      ||
 * 4+------7|
 *  0-------3
 *
 *  bottom (+z):
 *  0-------3
 *  |       |
 *  |       |
 *  |       |
 *  4-------7
 *
 * top (+z):
 *  5-------6
 *  |       |
 *  |       |
 *  |       |
 *  1-------2
 *
 * vertices:
 *  (0, 0, 0)
 *  (0, 1, 0)
 *  (1, 1, 0)
 *  (1, 0, 0)
 *  (0, 0, 1)
 *  (0, 1, 1)
 *  (1, 1, 1)
 *  (1, 0, 1)
 *
 * indices:
 *  7, 5, 4, 7, 6, 5 (south (+z))
 *  0, 2, 3, 0, 1, 2 (north (-z))
 *  3, 6, 7, 3, 2, 7 (east  (+x))
 *  4, 1, 0, 4, 5, 1 (west  (-x))
 *  1, 6, 2, 1, 5, 6 (up    (+y))
 *  4, 3, 7, 4, 0, 3 (down  (-y))
 */

constexpr auto INDICES =
    std::array<u16, 36>{
        7, 5, 4, 7, 6, 5, // (south (+z))
        0, 2, 3, 0, 1, 2, // (north (-z))
        3, 6, 7, 3, 2, 7, // (east  (+x))
        4, 1, 0, 4, 5, 1, // (west  (-x))
        1, 6, 2, 1, 5, 6, // (up    (+y))
        4, 3, 7, 4, 0, 3, // (down  (-y))
    };

constexpr auto VERTICES =
    std::array<vec3, 8>{
        vec3(0, 0, 0),
        vec3(0, 1, 0),
        vec3(1, 1, 0),
        vec3(1, 0, 0),
        vec3(0, 0, 1),
        vec3(0, 1, 1),
        vec3(1, 1, 1),
        vec3(1, 0, 1)
    };

constexpr auto TEX_COORDS =
    std::array<vec2, 4>{
        vec2(0, 0),
        vec2(1, 1),
        vec2(1, 0),
        vec2(0, 1),
    };

constexpr auto NORMALS =
    std::array<vec3, 6>{
        vec3(0, 0, 1),
        vec3(0, 0, -1),
        vec3(1, 0, 0),
        vec3(-1, 0, 0),
        vec3(0, 1, 0),
        vec3(0, -1, 0),
    };

// indices, within each list of 6 cube indices, which represent are the 4
// unique vertices which make up each face
constexpr auto UNIQUE_INDICES = std::array<u16, 4>{ 0, 1, 2, 4 };

// indices into emitted vertices which make up the two faces for a cube face
constexpr auto FACE_INDICES = std::array<u16, 6>{ 0, 1, 2, 0, 3, 1 };
}
