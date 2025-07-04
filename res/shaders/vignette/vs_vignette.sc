$input a_position, a_texcoord0
$output v_texcoord0, v_color0

#include "common.sc"

uniform vec4 u_color;

void main() {
	gl_Position = mul(u_modelViewProj, vec4(a_position.xyz, 1.0));
    v_color0 = u_color;
    v_texcoord0 = a_texcoord0;
}
