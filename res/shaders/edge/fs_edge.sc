$input v_texcoord0

#include "common.sc"

SAMPLER2D(s_input, 0);
SAMPLER2D(s_data, 1);

const float THRESHOLD = 0.0025;

uniform vec4 u_highlight_entities;

// edge by depth
float edge_depth(vec2 coord) {
	const vec2 texel = 1.0 / textureSize(s_input, 0);
    const float w = texel.x, h = texel.y;

    const float d = texture2D(s_input, coord).r;
    float result = 0.0;
    result += texture2D(s_input, coord + vec2( -w,  -h)).r > (d + THRESHOLD) ? 1.0 : 0.0;
    result += texture2D(s_input, coord + vec2(0.0,  -h)).r > (d + THRESHOLD) ? 1.0 : 0.0;
	result += texture2D(s_input, coord + vec2(  w,  -h)).r > (d + THRESHOLD) ? 1.0 : 0.0;
	result += texture2D(s_input, coord + vec2( -w, 0.0)).r > (d + THRESHOLD) ? 1.0 : 0.0;
    result += 1.0;
    result += texture2D(s_input, coord + vec2(  w, 0.0)).r > (d + THRESHOLD) ? 1.0 : 0.0;
	result += texture2D(s_input, coord + vec2( -w,   h)).r > (d + THRESHOLD) ? 1.0 : 0.0;
	result += texture2D(s_input, coord + vec2(0.0,   h)).r > (d + THRESHOLD) ? 1.0 : 0.0;
	result += texture2D(s_input, coord + vec2(  w,   h)).r > (d + THRESHOLD) ? 1.0 : 0.0;
    return result / 9.0;
}

void get_data(vec2 coord, out uint entity[8], out uint tile[8]) {
    const vec2 texel = 1.0 / textureSize(s_input, 0);
    const float w = texel.x, h = texel.y;

    vec4 data[8];
    data[0] = texture2D(s_data, coord + vec2( -w,  -h));
    data[1] = texture2D(s_data, coord + vec2(0.0,  -h));
	data[2] = texture2D(s_data, coord + vec2(  w,  -h));
	data[3] = texture2D(s_data, coord + vec2( -w, 0.0));
    data[4] = texture2D(s_data, coord + vec2(  w, 0.0));
	data[5] = texture2D(s_data, coord + vec2( -w,   h));
	data[6] = texture2D(s_data, coord + vec2(0.0,   h));
	data[7] = texture2D(s_data, coord + vec2(  w,   h));

    entity[0] = uint(data[0].b);
    entity[1] = uint(data[1].b);
	entity[2] = uint(data[2].b);
	entity[3] = uint(data[3].b);
    entity[4] = uint(data[4].b);
	entity[5] = uint(data[5].b);
	entity[6] = uint(data[6].b);
	entity[7] = uint(data[7].b);

    tile[0] = uint(data[0].a);
    tile[1] = uint(data[1].a);
	tile[2] = uint(data[2].a);
	tile[3] = uint(data[3].a);
    tile[4] = uint(data[4].a);
	tile[5] = uint(data[5].a);
	tile[6] = uint(data[6].a);
	tile[7] = uint(data[7].a);
}

// edge by tile id
float edge_tile(vec2 coord, uint ids[8]) {
    const uint id = uint(texture2D(s_data, coord).a);

    if (id == 0) {
        return 0.0;
    }

    // draw edges for tiles with a lower id than this tile
    float result = 1.0;
    for (uint i = 0; i < 8; i++) {
        result += (ids[i] != 0 && ids[i] < id) ? 1.0 : 0.0;
    }

    return result / 8.0;
}

// edges on entity IDs
// edges are outside of the highlighted entity
float edge_entity(vec2 coord, uint id, uint ids[8]) {
    const uint id_here = uint(texture2D(s_data, coord).b);

    if (id == 0 || id_here == id) {
        return 0.0;
    }

    // edges for all IDs that are not the same
    float result = 1.0;
    for (uint i = 0; i < 8; i++) {
        result += ids[i] == id ? 1.0 : 0.0;
    }

    return (result / 8.0) > 0.15 ? 1.0 : 0.0;
}

void main() {
    uint entity[8], tile[8];
    get_data(v_texcoord0, entity, tile);

    const float
        e_d = edge_depth(v_texcoord0) > 0.15 ? 1.0 : 0.0,
        e_t = edge_tile(v_texcoord0, tile) > 0.15 ? 1.0 : 0.0;

    gl_FragColor =
        clamp(
            vec4(
                edge_entity(v_texcoord0, uint(u_highlight_entities.r), entity),
                edge_entity(v_texcoord0, uint(u_highlight_entities.g), entity),
                edge_entity(v_texcoord0, uint(u_highlight_entities.b), entity),
                e_d + e_t
            ), vec4(0.0), vec4(1.0));
}
