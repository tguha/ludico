$input v_texcoord0, v_color0

#include "common.sc"

uniform vec4 u_intensity;

void main() {
    if (u_intensity.r < EPSILON
            || v_color0.a < EPSILON) {
        discard;
    }

    vec2 uv = v_texcoord0;
    uv *= vec2(1.0) - uv.ts;
    float v = uv.s * uv.t * 15.0;
    v = 1.0 - pow(v, u_intensity.r);
    gl_FragColor =
        vec4(
            v_color0.rgb,
            v_color0.a * v);
}
