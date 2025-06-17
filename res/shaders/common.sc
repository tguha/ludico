#ifndef COMMON_SC
#define COMMON_SC

#include "constants.hpp"

#ifndef __cplusplus

#include <bgfx_shader.sh>
#include <bgfx_compute.sh>

#define PI 3.1415926538
#define TAU (2 * PI)
#define EPSILON 1e-5

// used to represent vertices which ought to be discarded in the fragment
// shader. usually passed as a color value.
#define INF (1.0 / 0.0)
#define VERTEX_INVALID_VEC4 (vec4(INF, INF, INF, INF))
#define VERTEX_INVALID_VEC3 (vec3(INF, INF, INF))
#define VERTEX_INVALID_VEC2 (vec2(INF, INF))

#define DECL_CAMERA_UNIFORMS(prefix)        \
    uniform mat4 prefix ## _proj;           \
    uniform mat4 prefix ## _view;           \
    uniform mat4 prefix ## _viewProj;       \
    uniform mat4 prefix ## _invProj;        \
    uniform mat4 prefix ## _invView;        \
    uniform mat4 prefix ## _invViewProj;    \
    uniform vec4 prefix ## _position;       \
    uniform vec4 prefix ## _direction;

vec4 color_from_rgba(uint rgba) {
    return vec4(
        (((rgba <<  0) & 0xFF) / 255.0f),
        (((rgba <<  8) & 0xFF) / 255.0f),
        (((rgba << 16) & 0xFF) / 255.0f),
        (((rgba << 24) & 0xFF) / 255.0f));
}

float encode_u8(uint value) {
    return ((float) (value & 0xFF)) / 255.0;
}

uint decode_u8(float value) {
    return (uint) floor(value * 255.0);
}

vec4 encode_u32_v(uint value) {
    return vec4(
        encode_u8((value >>  0) & 0xFF),
        encode_u8((value >>  8) & 0xFF),
        encode_u8((value >> 16) & 0xFF),
        encode_u8((value >> 32) & 0xFF));
}

uint decode_u32_v(vec4 value) {
    return (decode_u8(value.x) <<  0)
        | (decode_u8(value.y)  <<  8)
        | (decode_u8(value.z)  << 16)
        | (decode_u8(value.w)  << 24);
}

float encode_u16(uint value) {
    return ((float) (value & 0xFFFF)) / 65536.0f;
}

uint decode_u16(float value) {
    return (uint) floor(value * 65536.0f);
}

float lindepth(float d, float near, float far) {
    return near * far / (far + d * (near - far));
}

// convert depth from NDC of either [0, 1] or [-1, 1] to [0, 1]
float to_normal_depth(float z) {
#if BGFX_SHADER_LANGUAGE_GLSL
    return (z * 0.5) + 0.5;
#else
    return z;
#endif
}

// converts clip coordinates to st coordinates
vec2 clip_to_st(vec2 clip) {
#if !BGFX_SHADER_LANGUAGE_GLSL
    clip.y = -clip.y;
#endif
    return clip;
}

float to_clip_depth(float z) {
#if BGFX_SHADER_LANGUAGE_GLSL
    return (z * 2.0) - 1.0;
#else
    return z;
#endif
}

vec2 clip_to_texture(vec2 clip) {
    clip = (clip * 0.5) + 0.5;
#if !BGFX_SHADER_LANGUAGE_GLSL
    clip.y = 1.0 - clip.y;
#endif
    return clip;
}

vec3 clip_from_st_depth(vec2 st, float depth) {
    vec3 clip = vec3((st * 2.0) - 1.0, to_clip_depth(depth));
#if !BGFX_SHADER_LANGUAGE_GLSL
    clip.y = -clip.y;
#endif
    return clip;
}

vec3 clip_to_view(mat4 inv_proj, vec3 clip) {
    vec4 pos_v = mul(inv_proj, vec4(clip, 1.0));
    return pos_v.xyz / pos_v.w;
}

vec3 clip_to_world(mat4 inv_view_proj, vec3 clip) {
    vec4 pos_w = mul(inv_view_proj, vec4(clip, 1.0));
    return pos_w.xyz / pos_w.w;
}

vec3 get_camera_pos_w(mat4 inv_view_proj) {
    return mul(inv_view_proj, vec4(0.5, 0.5, 0.0, 1.0)).xyz;
}

float noise(vec2 st) {
    return frac(sin(dot(st.xy, vec2(12.9898,78.233))) * 43758.5453123);
}

// adapted from: github.com/shff/opengl_sky/blob/master/main.c
float hash(float n) {
    return frac(sin(n) * 43758.5453123);
}

float round_multiple(float x, float m) {
    float r = mod(x, m);
    float f = r;
    float c = (m - r);
    return x - (abs(f) < abs(c) ? f : c);
}

vec3 round_multiple(vec3 x, vec3 m) {
    return vec3(
        round_multiple(x.x, m.x),
        round_multiple(x.y, m.y),
        round_multiple(x.z, m.z));
}

float intbound(float s, float ds) {
    if (ds < 0 && round(s) == s) {
        return 0;
    }

    float c = s == 0.0 ? 1.0 : ceil(s);
    return (ds > 0 ? (c - s) : (s - floor(s))) / abs(ds);
}

// get smallest possible t such that s + t * ds is an integer
vec3 intbound(vec3 s, vec3 ds) {
    return vec3(intbound(s.x, ds.x), intbound(s.y, ds.y), intbound(s.z, ds.z));
}

int dominant_axis(vec3 v) {
    return v.x > v.y ? (v.x > v.z ? 0 : 2) : (v.y > v.z ? 1 : 2);
}

#define GET_PASS_MVP()                                                         \
    ((((uint(u_render_flags.x) &                                               \
            (RENDER_FLAG_PASS_BASE | RENDER_FLAG_PASS_ENTITY_STENCIL)) != 0)   \
        && ((uint(u_render_flags.x) & RENDER_FLAG_CVP) != 0)) ?                \
        mul(u_cvp_viewProj, u_model[0]) : mul(u_viewProj, u_model[0]))

void write_gbuffers(
    out vec4 result[3],
    vec4 color,
    vec3 normal,
    float shine,
    uint flags,
    uint entity_id,
    uint tile_id) {
    result[0] = color;
    result[1] = vec4(normal, shine);
    result[2] = vec4(encode_u16(flags), 0.0, entity_id, tile_id);
}

// color, normal, shine, flags, entity_id, tile_id
#define WRITE_GBUFFERS(c, n, s, f, e, t) {                                     \
        vec4 res[3];                                                           \
        write_gbuffers(res, c, n, s, f, e, t);                                 \
        gl_FragData[0] = res[0];                                               \
        gl_FragData[1] = res[1];                                               \
        gl_FragData[2] = res[2];                                               \
    }

#endif
#endif
