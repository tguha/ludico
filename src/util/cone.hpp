#pragma once

#include "util/math.hpp"
#include "util/util.hpp"

namespace math {
struct Cone {
    vec3 tip, dir;
    f32 height, radius;

    Cone() = default;
    Cone(const vec3 &tip, const vec3 &dir, f32 height, f32 radius)
        : tip(tip),
          dir(math::normalize(dir)),
          height(height),
          radius(radius) {}
    Cone(const vec3 &top, const vec3 &bottom, f32 radius)
        : tip(top),
          dir(math::normalize(bottom - top)),
          height(math::length(bottom - top)),
          radius(radius) {}

    // returns true if cone contains point p
    inline bool contains(const vec3 &p) {
        // project p onto dir to find point's distance along axis
        const auto d = math::dot(p - this->tip, this->dir);

        // outside of axis
        if (d < 0 || d > this->height) {
            return false;
        }

        // calculate radius at point along axis
        const auto r = (d / this->height) * this->radius;

        // calculate point's orthogonal distance from axis
        const auto d_ortho = math::length((p - this->tip) - (d * this->dir));

        // compare against cone radius
        return d_ortho < r;
    }

    inline std::string to_string() const {
        return fmt::format(
            "Cone({},{},{},{})",
            this->tip, this->dir, this->height, this->radius);
    }
};
}
