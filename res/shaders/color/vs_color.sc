$input a_position, a_color0, a_normal
$output v_color0, v_normal

#include "common.sc"

uniform mat3 u_normal_mtx;

void main() {
	gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));
	v_color0 = a_color0;
    v_normal = normalize(mul(u_normal_mtx, a_normal).xyz) * 0.5 + 0.5;
}
