$input a_position, i_data0
$output v_texcoord0, v_texcoord1, v_normal, v_color0, v_color1

#include "common.sc"

// u_buffer_instance stride in floats
// must match size of ModelInstanceData + 1 (+1 is index offset for instance)
// TODO: check by constants/static assert
#define BUFFER_INSTANCE_STRIDE (28)
BUFFER_RO(u_buffer_instance, float, 5);  // BUFFER_INDEX_INSTANCE
BUFFER_RO(u_buffer_data, float, 6);      // BUFFER_INDEX_INSTANCE_DATA

#define BUFFER_VEC4(_b, _i)                                                    \
    (vec4(_b[(_i) + 0], _b[(_i) + 1], _b[(_i) + 2], _b[(_i) + 3]))

#define BUFFER_VEC3(_b, _i)                                                    \
    (vec3(_b[(_i) + 0], _b[(_i) + 1], _b[(_i) + 2]))

#define BUFFER_VEC2(_b, _i)                                                    \
    (vec2(_b[(_i) + 0], _b[(_i) + 1]))

#define BUFFER_FLOAT(_b, _i)                                                   \
    (_b[(_i)])

// offset/stride info for current mesh
// see macros for access/component meanings
uniform vec4 u_model_instance;
#define u_instance_offset   (u_model_instance.x)
#define u_index_offset      (u_model_instance.y)
#define u_vertex_offset     (u_model_instance.z)
#define u_vertex_stride     (u_model_instance.w)

void main() {
    CONST(uint) base_instance =
        uint(u_instance_offset)
            + (gl_InstanceID * BUFFER_INSTANCE_STRIDE)
            + uint(i_data0.x);

    // get model, normal matrices
    mat4 model =
        mtxFromCols(
            BUFFER_VEC4(u_buffer_instance, base_instance + 0),
            BUFFER_VEC4(u_buffer_instance, base_instance + 4),
            BUFFER_VEC4(u_buffer_instance, base_instance + 8),
            BUFFER_VEC4(u_buffer_instance, base_instance + 12));
    mat3 normal =
        mtxFromCols(
            BUFFER_VEC3(u_buffer_instance, base_instance + 16),
            BUFFER_VEC3(u_buffer_instance, base_instance + 19),
            BUFFER_VEC3(u_buffer_instance, base_instance + 22));
    uint flags =
        uint(BUFFER_FLOAT(u_buffer_instance, base_instance + 25));
    uint id =
        uint(BUFFER_FLOAT(u_buffer_instance, base_instance + 26));

    // starting index for this instance
    uint index_offset =
        uint(BUFFER_FLOAT(u_buffer_instance, base_instance + 27));

    // read index into vertex data
    uint index =
        uint(
            u_buffer_data[
                uint(u_index_offset) + index_offset + gl_VertexID]);

    if (index == INDEX_INVALID) {
        gl_Position = vec4(0.0);
        v_texcoord0 = VERTEX_INVALID_VEC2;
        v_texcoord1 = VERTEX_INVALID_VEC2;
        v_normal = VERTEX_INVALID_VEC3;
        return;
    }

    // get offset into vertex data
    uint vertex_offset =
        uint(u_vertex_offset) + (index * uint(u_vertex_stride));

    // read position, tex, spec from data buffer
    vec3 d_position = BUFFER_VEC3(u_buffer_data, vertex_offset + 0);
    vec2 d_tex = BUFFER_VEC2(u_buffer_data, vertex_offset + 3);
    vec2 d_spec = BUFFER_VEC2(u_buffer_data, vertex_offset + 5);
    vec3 d_normal = BUFFER_VEC3(u_buffer_data, vertex_offset + 7);

    gl_Position = mul(mul(u_viewProj, model), vec4(d_position, 1.0));
	v_texcoord0 = d_tex;
	v_texcoord1 = d_spec;
    v_normal = normalize(mul(normal, d_normal).xyz) * 0.5 + 0.5;
    v_color0 = flags;
    v_color1 = id;
}
