$input a_position, a_normal, a_texcoord0, a_texcoord1, a_color0
$output v_texcoord0, v_texcoord1, v_normal, v_position, v_color0

#include "common.sc"

uniform vec4 u_render_flags;

DECL_CAMERA_UNIFORMS(u_cvp)

void main() {
    mat4 mvp;

    bool
        base_pass = (uint(u_render_flags.x) & RENDER_FLAG_PASS_BASE) != 0,
        cvp = (uint(u_render_flags.x) & RENDER_FLAG_CVP) != 0;

    if (base_pass && cvp) {
        mvp = mul(u_cvp_viewProj, u_model[0]);
    } else {
        mvp = mul(u_viewProj, u_model[0]);
    }

    gl_Position = mul(mvp, vec4(a_position, 1.0));

    v_texcoord0 = a_texcoord0;
    v_texcoord1 = a_texcoord1;
    //v_color0 = uint(a_color0);
    v_normal = normalize(mul(u_model[0], vec4(a_normal, 0.0)).xyz) * 0.5 + 0.5;
    v_position = mul(u_model[0], vec4(a_position, 1.0)).xyz;
}
