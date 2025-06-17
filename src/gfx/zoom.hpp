#pragma once

#include "util/math.hpp"

enum class Zoom {
    ZOOM_0 = 0,
    ZOOM_1 = 1,
    ZOOM_2 = 2,
    ZOOM_3 = 3,
    ZOOM_4 = 4,
    ZOOM_5 = 5,
    ZOOM_6 = 6,
    ZOOM_7 = 7,
    ZOOM_8 = 8,
    ZOOM_DEFAULT = ZOOM_0,

    ZOOM_MIN = ZOOM_0,
    ZOOM_MAX = ZOOM_8
};

inline Zoom &operator++(Zoom &z) {
    return z =
        static_cast<Zoom>(
            math::clamp(
                static_cast<int>(z) + 1,
                static_cast<int>(Zoom::ZOOM_MIN),
                static_cast<int>(Zoom::ZOOM_MAX)));
}

inline Zoom &operator--(Zoom &z) {
    return z =
        static_cast<Zoom>(
            math::clamp(
                static_cast<int>(z) - 1,
                static_cast<int>(Zoom::ZOOM_MIN),
                static_cast<int>(Zoom::ZOOM_MAX)));
}
