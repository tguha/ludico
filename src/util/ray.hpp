#pragma once

#include "util/log.hpp"
#include "util/types.hpp"
#include "util/math.hpp"
#include "util/aabb.hpp"
#include "util/direction.hpp"
#include "math.hpp"

namespace math {
struct Ray {
    vec3 origin, direction;

    Ray() = default;
    Ray(const vec3& o, const vec3 &d)
        : origin(o), direction(d) {}

    std::string to_string() const {
        return fmt::format("Ray({},{})", this->origin, this->direction);
    }

    inline bool valid() const {
        return
            !math::any(
                math::isnan(this->origin)
                | math::isinf(this->origin)
                | math::isnan(this->direction)
                | math::isinf(this->direction));
    }

    // ray-point distance
    inline f32 distance(const vec3 &p) const {
        return math::length(p - this->project(p));
    }

    // project point onto ray
    inline vec3 project(const vec3 &p) const {
        return this->origin
            + (math::dot(this->direction, p - this->origin) * this->direction);
    }

    inline std::optional<vec3> intersect_aabb(
        const AABB &aabb,
        std::optional<vec3*> other_hit = std::nullopt) const {
        if (!this->valid()) {
            return std::nullopt;
        }
        // very fast! from
        // gamedev.stackexchange.com/questions/18436/most-efficient-aabb-vs-ray-collision-algorithms
        const auto dirfrac = 1.0f / this->direction;
        const auto
            t1 = (aabb.min.x - this->origin.x) * dirfrac.x,
            t2 = (aabb.max.x - this->origin.x) * dirfrac.x,
            t3 = (aabb.min.y - this->origin.y) * dirfrac.y,
            t4 = (aabb.max.y - this->origin.y) * dirfrac.y,
            t5 = (aabb.min.z - this->origin.z) * dirfrac.z,
            t6 = (aabb.max.z - this->origin.z) * dirfrac.z,
            tmin =
                math::max(
                    math::max(
                        math::min(t1, t2), math::min(t3, t4)),
                    math::min(t5, t6)),
            tmax =
                math::min(
                    math::min(
                        math::max(t1, t2), math::max(t3, t4)),
                    math::max(t5, t6));

        // tmax < 0, ray intersects AABB, but it is behind origin
        if (tmax < 0) {
            return std::nullopt;
        }

        // tmin > tmax, no intersection
        if (tmin > tmax) {
            return std::nullopt;
        }

        // intersection, return hit point
        if (other_hit) {
            **other_hit = this->origin + (this->direction * tmax);
        }

        return this->origin + (this->direction * tmin);
    }

    template <typename F>
    inline std::optional<std::tuple<ivec3, Direction::Enum>> intersect_block(
        F &&f,
        f32 max_distance,
        std::optional<vec3*> hit_out = std::nullopt) const {
        if (!this->valid()) {
            return std::nullopt;
        }
        auto pos = this->origin;
        f32 d = 0.0f;

        const auto
            d_s = math::sign(this->direction),
            d_a = math::abs(this->direction);

        while (d < max_distance) {
            // axis distance to nearest cell
            auto dist = math::fract(-pos * d_s) + vec3(1e-4);

            // ray distance to each axis
            auto len = dist / d_a;

            // pick shortest axis
            const auto axis =
                len.x < len.y ?
                    ((len.x < len.z) ? 0 : 2)
                    : ((len.y < len.z) ? 1 : 2);
            const auto near = len[axis];

            // step to nearest voxel cell
            pos += this->direction * near;
            d += near;

            const auto pos_i = ivec3(math::floor(pos));
            if (f(pos_i)) {
                if (hit_out) {
                    **hit_out = pos;
                }

                const auto dir =
                    Direction::from_axis(
                        axis, -math::sign(this->direction[axis]));
                return std::make_optional(std::make_tuple(pos_i, dir));
            }
        }

        return std::nullopt;
    }
};
}
