#pragma once

#include "util/types.hpp"
#include "util/util.hpp"
#include "util/camera.hpp"

#include "gfx/bgfx.hpp"

struct Texture;
struct Program;

struct Sun {
    Camera camera;
    vec3 direction;
    vec3 diffuse, ambient;

    Sun() = default;

    void update(const Texture &depth, const Camera &camera);

    UniformValueList &uniforms(UniformValueList &dst) const;
};
