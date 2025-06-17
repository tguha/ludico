#pragma once

#include "util/types.hpp"
#include "util/math.hpp"
#include "serialize/annotations.hpp"

// TODO: reimplement as LerpVal
// storage for an automatically interpolated direction vector
// also provides convenience functions for "downsampling" into 16-direction
// rotation
struct LerpDir {
    LerpDir() = default;
    explicit LerpDir(usize lerping_ticks)
        : lerping_ticks(lerping_ticks) {}

    inline LerpDir &operator=(const vec3 &rhs) {
        this->v = math::normalize(rhs);
        return *this;
    }

    void tick() {
        this->v_lerp =
            this->v_lerp +
                ((this->raw() - this->v_lerp) / f32(lerping_ticks));
    }

    inline vec3 raw() const {
        return math::any(math::isnan(this->v) | math::isinf(this->v)) ?
            vec3(1.0f, 0.0f, 0.0f)
            : this->v;
    }

    inline vec3 lerped() const {
        return math::any(math::isnan(this->v_lerp) | math::isinf(this->v_lerp)) ?
            vec3(1.0f, 0.0f, 0.0f)
            : this->v_lerp;
    }

    inline f32 xz_angle() const {
        return math::atan2<f32, math::defaultp>(this->v.x, this->v.z);
    }

    inline vec3 xz_dir() const {
        return math::xz_angle_to_dir(this->xz_angle());
    }

    inline f32 xz_angle_lerp() const {
        return math::atan2<f32, math::defaultp>(
            this->v_lerp.x, this->v_lerp.z);
    }

    inline vec3 xz_dir_lerp() const {
        return math::xz_angle_to_dir(this->xz_angle_lerp());
    }

    inline f32 xz_angle_lerp16() const {
        return math::floorMultiple(
            math::atan2<f32, math::defaultp>(
                this->v_lerp.x, this->v_lerp.z) + 0.01f,
            math::PI_8);
    }

    inline vec3 xz_dir_lerp16() const {
        return math::xz_angle_to_dir(this->xz_angle_lerp16());
    }

private:
    usize lerping_ticks;
    vec3 v = vec3(0.0f);
    vec3 v_lerp = vec3(0.0f);
};
