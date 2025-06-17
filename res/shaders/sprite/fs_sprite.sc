$input v_texcoord0, v_color0, v_color1

#include "common.sc"

SAMPLER2D(s_tex, 0);

void main() {
    vec4 color = texture2D(s_tex, v_texcoord0);

    if (color.a < EPSILON) {
        discard;
    }

    gl_FragColor = vec4(mix(color.rgb, v_color0.rgb, v_color0.a), v_color1.r);
}
