$input a_position, i_data0
$output v_texcoord0, v_color0, v_color1

#include "common.sc"

BUFFER_RO(u_sprite_buffer, vec4, 4); // BUFFER_INDEX_SPRITE

#define STRIDE 4

// offset in vec4s into u_sprite_buffer
uniform vec4 u_offset;

void main() {
    int id = int(i_data0.w);
    uint offset = uint(u_offset[0]);
    vec4 pos_size = u_sprite_buffer[offset + (gl_InstanceID * STRIDE) + 0 + id];
    vec4 color = u_sprite_buffer[offset + (gl_InstanceID * STRIDE) + 1].xyzw;
    vec4 alpha = u_sprite_buffer[offset + (gl_InstanceID * STRIDE) + 2];
    vec4 stmm = u_sprite_buffer[offset + (gl_InstanceID * STRIDE) + 3];
    vec2 pos = pos_size.xy;
    vec2 size = pos_size.zw;
    mat4 m = mtxFromCols(
        vec4(1.0, 0.0, 0.0, 0.0),
        vec4(0.0, 1.0, 0.0, 0.0),
        vec4(0.0, 0.0, 1.0, 0.0),
        vec4(pos, 0.0, 1.0));

	gl_Position = mul(mul(u_viewProj, m), vec4(a_position.xy * size, 0.0, 1.0));
    v_color0 = color;
    v_color1 = alpha;
	v_texcoord0 = stmm.xy + ((stmm.zw - stmm.xy) * a_position.xy);
}
