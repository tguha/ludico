$input v_texcoord0, v_texcoord1, v_normal, v_position, v_color0

#include "common.sc"

SAMPLER2D(s_tex, 0);

uniform vec4 u_color;

uint axis(vec3 v) {
    if (abs(v.x) > EPSILON) {
        return 0;
    } else if (abs(v.y) > EPSILON) {
        return 1;
    }

    return 2;
}

void main() {
    vec4 color = texture2D(s_tex, v_texcoord0);

    if (color.a < EPSILON) {
        discard;
    }

    vec3 n = (v_normal * 2.0f) - 1.0f;

    float brightness = 1.0;

    if (axis(n) == 0) {
        brightness = 0.5;
    } else if (axis(n) == 2) {
        brightness = 0.7;
    } else if (axis(n) == 1 && n.y < 0) {
        brightness = 0.4;
    }

    WRITE_GBUFFERS(
        vec4(mix(brightness * color.rgb, u_color.rgb, u_color.a), 1.0),
        v_normal,
        texture2D(s_tex, v_texcoord1).r,
        0,
        0,
        v_color0);
}
