#ifndef LIGHT_SC
#define LIGHT_SC

#include "constants.hpp"

// size of packed light data, in vec4
#define LIGHT_SIZE_VEC4 5

#define LIGHT_UNIFORM_NAME u_lights

#ifndef __cplusplus

#ifdef DECL_LIGHT_UNIFORMS
// TODO: BUFFER_INDEX_LIGHT
BUFFER_RO(LIGHT_UNIFORM_NAME, vec4, 2);
#endif

struct Light {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    vec3 att;
    bool shadow;
    bool present;
};

// unpack light from light uniform
void get_light(uint i, out Light l) {
    const uint base = i * LIGHT_SIZE_VEC4;
    l.position  = LIGHT_UNIFORM_NAME[base + 0].xyz;
    l.present   = LIGHT_UNIFORM_NAME[base + 0].w > 0.0;
    l.att       = LIGHT_UNIFORM_NAME[base + 1].rgb;
    l.shadow    = LIGHT_UNIFORM_NAME[base + 1].w > 0.0;
    l.ambient   = LIGHT_UNIFORM_NAME[base + 2].rgb;
    l.diffuse   = LIGHT_UNIFORM_NAME[base + 3].rgb;
    l.specular  = LIGHT_UNIFORM_NAME[base + 4].rgb;
}

#endif

#endif
