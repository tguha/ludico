$input a_position, a_texcoord0, a_texcoord1, a_normal
$output v_texcoord0, v_texcoord1, v_normal

#include "common.sc"

uniform mat3 u_normal_mtx;

DECL_CAMERA_UNIFORMS(u_cvp)
uniform vec4 u_render_flags;

void main() {
	gl_Position = mul(GET_PASS_MVP(), vec4(a_position, 1.0));
	v_texcoord0 = a_texcoord0;
	v_texcoord1 = a_texcoord1;
    v_normal = normalize(mul(u_normal_mtx, a_normal).xyz) * 0.5 + 0.5;
}
