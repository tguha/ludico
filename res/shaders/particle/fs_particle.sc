$input v_color0, v_color1

#include "common.sc"

uniform vec4 u_render_flags;

void main() {
    vec4 color = v_color0;

    if ((uint(u_render_flags.x) & RENDER_FLAG_PASS_TRANSPARENT) == 0) {
        color.rgb *= color.a;
    }

    WRITE_GBUFFERS(
        v_color0,
        vec3(0.0, 1.0, 0.0),
        v_color1,
        GFX_FLAG_NOEDGE,
        0,
        0);
}
