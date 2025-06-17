#include "model/prelude_fs.sc"

uniform vec4 u_shrine_activated_ticks;
uniform vec4 u_shrine_color;
uniform vec4 u_ticks;

CONST(uint) FADE_IN_TICKS = 60;
CONST(vec3) COLOR_PLACEHOLDER = vec3(1.0, 0.0, 0.0);

bool main_ext(inout vec4 color, inout float specular, inout vec4 flags_id) {
    CONST(vec4) color_base = color_from_rgba(0xFF666666);

    if (all(equal(color.rgb, COLOR_PLACEHOLDER))) {
        if (u_ticks.x >= u_shrine_activated_ticks.x) {
            float fade =
                clamp(
                    (u_ticks.x - u_shrine_activated_ticks.x)
                        / float(FADE_IN_TICKS),
                    0.0,
                    1.0);
            color = vec4(
                lerp(color_base, 1.2f * u_shrine_color.rgb, fade),
                color.a);
            specular = 1.25f;
        } else {
            color = vec4(color_base.rgb, color.a);
        }
    }

    return true;
}

#define FS_MODEL_EXTENSION
#include "model/fs_model.sc"
