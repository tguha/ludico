$input v_texcoord0, v_texcoord1, v_normal, v_position, v_color0

#include "common.sc"

SAMPLER2D(s_tex, 0);

uniform vec4 u_color;
uniform vec4 u_alpha;
uniform vec4 u_flags_id;

void main() {
    vec4 color = texture2D(s_tex, v_texcoord0);

    if (color.a < EPSILON) {
        discard;
    }

    WRITE_GBUFFERS(
        vec4(mix(color.rgb, u_color.rgb, u_color.a), u_alpha.a),
        v_normal,
        texture2D(s_tex, v_texcoord1).r,
        uint(u_flags_id.x),
        uint(u_flags_id.y),
        int(u_flags_id.y) == 0 ? 0 : v_color0);
}
