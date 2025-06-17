#pragma once

#include "gfx/bgfx.hpp"
#include "util/types.hpp"
#include "util/math.hpp"
#include "util/aabb.hpp"
#include "util/list.hpp"
#include "util/allocator.hpp"

struct Program;
struct UniformValueList;

struct Camera {
    mat4 view, proj, view_proj, inv_proj, inv_view, inv_view_proj;
    vec3 position, direction;

    Camera() = default;
    Camera(const mat4 &view, const mat4 &proj)
        : view(view),
          proj(proj) {
        this->update();
    }

    inline vec3 project(const vec3 &pos) const {
        return (this->view_proj * vec4(pos, 1.0f)).xyz();
    }

    inline ivec2 to_pixel(const vec3 &pos, const uvec2 &size) const {
        return ivec2(
            math::round(
                (((this->view_proj * vec4(pos, 1.0f)).xy() + 1.0f) / 2.0f)
                    * vec2(size)));
    }

    inline void set_view_transform(bgfx::ViewId _view) const {
        bgfx::setViewTransform(
            _view,
            math::value_ptr(view),
            math::value_ptr(proj));
    }

    inline std::array<vec3, 8> world_space_points() const {
#ifdef DEPTH_ZERO_TO_ONE
        const auto depth_range = vec2(0, 1);
#elif
        const auto depth_range = vec2(-1, 1);
#endif
        const auto ndc =
            types::make_array(
                vec3(-1, -1, depth_range.x),
                vec3(-1,  1, depth_range.x),
                vec3( 1,  1, depth_range.x),
                vec3( 1, -1, depth_range.x),
                vec3(-1, -1, depth_range.y),
                vec3(-1,  1, depth_range.y),
                vec3( 1,  1, depth_range.y),
                vec3( 1, -1, depth_range.y));
        const auto inv_view_proj = math::inverse(this->proj * this->view);

        auto result = std::array<vec3, 8>();
        std::transform(
            ndc.begin(), ndc.end(), result.begin(),
            [&](const auto &v) { return vec3(inv_view_proj * vec4(v, 1.0f)); });
        return result;
    }

    inline AABB world_space_bounds() const {
        auto ps = this->world_space_points();
        return AABB::compute(std::span(ps));
    }

    // retrieve uniforms for this camera
    UniformValueList &uniforms(
        UniformValueList &dst,
        std::string_view prefix) const;

    // update calculated matrices
    void update();
};
