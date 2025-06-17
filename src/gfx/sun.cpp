#include "gfx/sun.hpp"
#include "gfx/program.hpp"
#include "gfx/texture.hpp"

void Sun::update(
    const Texture &depth,
    const Camera &camera) {
    ASSERT(
        depth.size.x == depth.size.y,
        "sun depth texture must be square, not {}", depth.size);
    const f32 texture_size = math::compMax(depth.size);

    // compute sun bounds from camera bounds
    auto points_w = camera.world_space_points();
    const auto bounds_w = AABB::compute(std::span(points_w));
    const auto center_w = bounds_w.center();

    // view is world space center looking at center + direction
    const auto v =
        math::lookAt(
            center_w,
            center_w + this->direction,
            vec3(0.0f, 1.0f, 0.0f));

    // convert bounding points into light space points
    auto points_v = std::array<vec3, 8>();
    std::transform(
        points_w.begin(), points_w.end(), points_v.begin(),
        [&](auto &q) { return vec3(v * vec4(q, 1.0f)); });

    // use min/max in light space to determine ortho bounds
    const auto bounds_v = AABB::compute(std::span(points_v));

    this->camera.proj =
        math::ortho(
            bounds_v.min.x, bounds_v.max.x,
            bounds_v.min.y, bounds_v.max.y,
            bounds_v.min.z, bounds_v.max.z);
    this->camera.view = v;

    // fix shadow shimmering
    // stackoverflow.com/questions/33499053/cascaded-shadow-map-shimmering
    auto origin_s =
        (this->camera.proj * this->camera.view) *
            vec4(vec3(0.0), 1.0);
    origin_s *= texture_size / 2.0f;

    auto origin_r = math::round(origin_s);
    auto offset = origin_r - origin_s;

    offset *= 2.0f / texture_size;
    offset.z = 0;
    offset.w = 0;

    this->camera.proj[3] += offset;
    this->camera.update();
}

UniformValueList &Sun::uniforms(UniformValueList &dst) const {
    this->camera.uniforms(dst, "u_look_sun");
    dst.set("u_sun_direction", vec4(this->direction, 0.0));
    dst.set("u_sun_diffuse", vec4(this->diffuse, 1.0));
    dst.set("u_sun_ambient", vec4(this->ambient, 1.0));
    return dst;
}
