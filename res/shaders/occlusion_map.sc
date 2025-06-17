#ifndef OCCLUSION_MAP_SC
#define OCCLUSION_MAP_SC

#include "common.sc"

#define OCCM_FULL_SAMPLER_NAME s_occm_full
#define OCCM_TOP_SAMPLER_NAME s_occm_top
#define OCCM_BLOCKING_SAMPLER_NAME s_occm_blocking
#define OCCM_OFFSET_UNIFORM_NAME u_occm_offset
#define OCCM_SIZE_UNIFORM_NAME u_occm_size

#ifndef __cplusplus

#ifdef OCCM_DECL_UNIFORMS
uniform vec4 OCCM_SIZE_UNIFORM_NAME;
uniform vec4 OCCM_OFFSET_UNIFORM_NAME;
#endif

// convert world position to 3D occlusion map sample coordinate
vec3 _occm_sample_pos_3d(vec3 pos_w) {
    vec3 q = pos_w;
    q -= OCCM_OFFSET_UNIFORM_NAME.xyz;
    q /= OCCM_SIZE_UNIFORM_NAME.xyz;
    q -= mod(q, 1.0 / OCCM_SIZE_UNIFORM_NAME.xyz);
    q += 0.5 / OCCM_SIZE_UNIFORM_NAME.xyz;
    return q;
}

vec2 _occm_sample_pos_2d(vec2 pos_xz_w) {
    vec2 q = pos_xz_w;
    q -= OCCM_OFFSET_UNIFORM_NAME.xz;
    q /= OCCM_SIZE_UNIFORM_NAME.xz;
    q -= mod(q, 1.0 / OCCM_SIZE_UNIFORM_NAME.xz);
    q += 0.5 / OCCM_SIZE_UNIFORM_NAME.xz;
    return q;
}

bool occm_sample(vec3 pos_w) {
    return texture3D(
        OCCM_FULL_SAMPLER_NAME,
        _occm_sample_pos_3d(pos_w)).r > EPSILON;
}

bool occm_sample_top(vec2 pos_xz_w) {
    return texture2D(
        OCCM_TOP_SAMPLER_NAME,
        _occm_sample_pos_2d(pos_xz_w)).r > EPSILON;
}

bool occm_sample_blocking(vec3 pos_w) {
    return texture3D(
        OCCM_BLOCKING_SAMPLER_NAME,
        _occm_sample_pos_3d(pos_w)).r > EPSILON;
}

// raycast algorithm borrowed from
// https://www.shadertoy.com/view/fstSRH
bool occm_raycast(
    vec3 origin,
    vec3 direction,
    float max_distance,
    uint n_hits,
    out vec3 hit) {
    vec3 pos = origin, p;
    float d = 0.0;
    uint count = 0;

    vec3 d_s = sign(direction);
    vec3 d_a = abs(direction);

    while (d < max_distance) {
        // axis distance to nearest cell
        vec3 dist = fract(-pos * d_s) + 1e-4;

        // ray distance to each axis as vec3
        vec3 len = dist / d_a;
        vec3 near = min(len.x, min(len.y, len.z));

        // step to nearest voxel cell
        pos += direction * near;
        d += near;

        // don't worry about rounding pos, this is done in occm_sample
        if (occm_sample(pos)) {
            hit = pos;

            count++;
            if (count == n_hits) {
                return true;
            }
        }
    }

    return false;
}

#endif // ifndef __cplusplus
#endif // ifndef OCCLUSION_MAP_SC
