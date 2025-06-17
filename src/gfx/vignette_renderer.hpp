#pragma once

#include "util/util.hpp"
#include "util/types.hpp"
#include "util/math.hpp"
#include "gfx/util.hpp"

// screen-space vignette renderer
struct VignetteRenderer {
    static void render(
        f32 intensity,
        const vec4 &color,
        RenderState render_state);
};
