$input a_position, i_data0
$output v_color0, v_color1

#include "common.sc"

BUFFER_RO(u_particle_buffer, vec4, 3); // BUFFER_INDEX_PARTICLE

void main() {
    int id = int(i_data0.w);
    vec4 u0 = u_particle_buffer[(gl_InstanceID * 2) + 0 + id].xyzw;
    vec3 pos = u0.xyz;
    float shine = u0.w;
    vec4 color = u_particle_buffer[(gl_InstanceID * 2) + 1].rgba;
    mat4 m = mtxFromCols(
        vec4(1.0, 0.0, 0.0, 0.0),
        vec4(0.0, 1.0, 0.0, 0.0),
        vec4(0.0, 0.0, 1.0, 0.0),
        vec4(vec3(pos), 1.0));
	gl_Position = mul(mul(u_viewProj, m), vec4(a_position, 1.0));
    v_color0 = color;
    v_color1 = shine;
}
