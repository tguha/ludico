$input v_texcoord0

#include "common.sc"

SAMPLER2D(s_tex, 0);

uniform vec4 u_game_camera_sample_offset;
uniform vec4 u_game_camera_sample_range;

void main() {
    vec4 color =
        texture2D(
            s_tex,
            u_game_camera_sample_offset +
                (v_texcoord0 * u_game_camera_sample_range.xy));

    if (color.a < EPSILON) {
        discard;
    }

    gl_FragColor = vec4(color.rgb, 1.0);
}
