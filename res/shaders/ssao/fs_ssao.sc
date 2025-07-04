$input v_texcoord0

#include "common.sc"

SAMPLER2D(s_normal, 0);
SAMPLER2D(s_depth, 1);
SAMPLER2D(s_noise, 2);

DECL_CAMERA_UNIFORMS(u_look_ssao)

uniform vec4 ssao_samples[64];

float rand(vec2 co) {
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

float ssao(vec2 st, vec3 pos_v, vec3 n_v, vec3 r) {
    const float radius = 0.1, bias = 0.0015;

    vec3 tangent = normalize(r - n_v * dot(r, n_v));
    vec3 bitangent = cross(n_v, tangent);
    mat3 tbn = mtxFromCols(tangent, bitangent, n_v);

    float z = 0;
    for (int i = 0; i < SSAO_SAMPLES; i++) {
        vec3 s = pos_v.xyz + (mul(tbn, ssao_samples[i].xyz) * radius);

        // transform into screen space
        vec4 o = mul(u_look_ssao_proj, vec4(s, 1.0));
        o.xyz /= o.w;
        o.xy = clip_to_texture(o.xy);

        float d = clip_to_view(
            u_look_ssao_invProj,
            clip_from_st_depth(
                st,
                texture2D(s_depth, o.xy).r)).z;
        float range = smoothstep(0.0, 1.0, radius / abs(pos_v.z - d));
        z += range * (d >= s.z + bias ? 1.0 : 0.0);
    }

    z /= SSAO_SAMPLES;
    z = pow(z, 1.4);
    z = 1.0 - z;
    z = clamp(z, 0.0, 1.0);
    return z;
}

void main() {
    float d = texture2D(s_depth, v_texcoord0).r;

    if (d > 0.9999) {
        gl_FragColor = vec4(1.0);
        return;
    }

    vec3 pos_v =
        clip_to_view(
            u_look_ssao_invProj,
            clip_from_st_depth(v_texcoord0, d));
    vec3 pos_w = mul(u_look_ssao_invView, vec4(pos_v, 1.0));
    vec4 t_n = texture2D(s_normal, v_texcoord0);
    vec3
        n_w = normalize((t_n.rgb * 2.0) - 1.0),
        n_v = normalize(mul(u_look_ssao_view, vec4(n_w, 0.0)).xyz);

    vec3 noise = texture2D(s_noise, vec2(v_texcoord0 / textureSize(s_noise, 0)));
    vec3 r = normalize(vec3(noise.xy, 0.0));

    gl_FragColor = vec4(vec3(ssao(v_texcoord0, pos_v, n_v, r)), 1.0);
}
