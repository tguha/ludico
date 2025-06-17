$input v_texcoord0

#include "../common.sc"

#define DECL_LIGHT_UNIFORMS
#include "light.sc"
#include "dither.sc"

#define OCCM_DECL_UNIFORMS
SAMPLER3D(s_occm_full, 10);
SAMPLER2D(s_occm_top, 11);
SAMPLER3D(s_occm_blocking, 12);
#include "occlusion_map.sc"

SAMPLER2D(s_gbuffer, 0);
SAMPLER2D(s_normal, 1);
SAMPLER2D(s_data, 2);
SAMPLER2D(s_depth, 3);
SAMPLER2D(s_gbuffer_tr, 4);
SAMPLER2D(s_normal_tr, 5);
SAMPLER2D(s_data_tr, 6);
SAMPLER2D(s_depth_tr, 7);
SAMPLER2D(s_sun, 8);
SAMPLER2D(s_noise, 9);

uniform vec4 u_sun_direction;
uniform vec4 u_sun_ambient;
uniform vec4 u_sun_diffuse;

DECL_CAMERA_UNIFORMS(u_look_light)
DECL_CAMERA_UNIFORMS(u_look_sun)

// TODO: defined by uniform or #define or something
CONST(vec2 POISSON[32]) = {
    { -0.94201624,  -0.39906216 },
    {  0.94558609,  -0.76890725 },
    { -0.094184101, -0.92938870 },
    {  0.34495938,   0.29387760 },
    { -0.91588581,   0.45771432 },
    { -0.81544232,  -0.87912464 },
    { -0.38277543,   0.27676845 },
    {  0.97484398,   0.75648379 },
    {  0.44323325,  -0.97511554 },
    {  0.53742981,  -0.47373420 },
    { -0.26496911,  -0.41893023 },
    {  0.79197514,   0.19090188 },
    { -0.24188840,   0.99706507 },
    { -0.81409955,   0.91437590 },
    {  0.19984126,   0.78641367 },
    {  0.14383161,  -0.14100790 },
    {  0.94201624,   0.39906216 },
    { -0.94558609,   0.76890725 },
    {  0.094184101,  0.92938870 },
    { -0.34495938,  -0.29387760 },
    {  0.91588581,  -0.45771432 },
    {  0.81544232,   0.87912464 },
    {  0.38277543,  -0.27676845 },
    { -0.97484398,  -0.75648379 },
    { -0.44323325,   0.97511554 },
    { -0.53742981,   0.47373420 },
    {  0.26496911,   0.41893023 },
    { -0.79197514,  -0.19090188 },
    {  0.24188840,  -0.99706507 },
    { -0.81409955,  -0.91437590 },
    { -0.19984126,  -0.78641367 },
    { -0.14383161,   0.14100790 },
};

#define SHADOW_EDGE_THRESHOLD 0.4

float shadow(vec3 pos_w, float bias, float cos_theta) {
    vec4 pos_s = mul(u_look_sun_viewProj, vec4(pos_w, 1.0));
    pos_s.xyz /= pos_s.w;
    vec2 st_s = clip_to_texture(pos_s.xy);
    pos_s.z = to_normal_depth(pos_s.z);

    // slope-scaled depth bias: as cos_theta approaches zero, bias the shadow
    // more
    float b = bias * tan(acos(cos_theta));
    float s = 0.0;
    vec2 texel = 1.0 / textureSize(s_sun, 0);
    for (int i = 0; i < SHADOW_SAMPLES; i++) {
        float d =
            texture2D(
                s_sun,
                st_s + (POISSON[i] * texel)).r;
        s += pos_s.z - b > d ? 1.0 : 0.0;
    }

    s = 1.0 - (s / (float) (SHADOW_SAMPLES));
    return s;
}

vec3 sunlight(vec3 view_dir, vec3 pos_w, vec3 n, float shine) {
    vec3 pos_l = mul(u_look_sun_view, vec4(pos_w, 1.0)).xyz;
    vec3 dir_l = normalize(u_sun_direction.xyz);
    vec3 dir_h = normalize(-dir_l + view_dir);

    float spec = shine < EPSILON ? 0.0 : pow(max(dot(n, dir_h), 0.0), shine);

    // cos_theta ranges from [-1, 1]: -1 when 'most' light, 1 when least light
    // if dir_l and n are complete opposites (dot is -1), then the surface is
    // getting maximum light
    float cos_theta = dot(dir_l, n);
    float d = max(-cos_theta, 0.0);
    float s = d > 0.0 ? shadow(pos_w, 0.00005, cos_theta) : 0.0;

    return (s * d * u_sun_diffuse.rgb)
        + (s * u_sun_diffuse.rgb * spec)
        + u_sun_ambient.rgb;
}

vec3 light(vec3 view_dir, vec3 pos_w, vec3 n_w, float shine) {
    vec3 result;

    for (uint i = 0; i < MAX_LIGHTS; i++) {
        Light l;
        get_light(i, l);

        // light not present, must be last in array
        if (!l.present) {
            break;
        }

        vec3 vec_l = l.position - pos_w;
        float dist = length(vec_l);
        float att = 1.0 / (l.att.x + l.att.y * dist + l.att.z * (dist * dist));

        // attenutation so low light is unnoticable
        if (att < 0.01) {
            continue;
        }

        vec3 dir_l = normalize(vec_l);

        // raycast for shadow check
        vec3 hit;
        if (l.shadow
                && occm_raycast(
                    l.position, -dir_l, LIGHT_MAX_VALUE + 1.0, 1, hit)) {
            // add small bias since hit and dist can be very close
            if (length(l.position - hit) + 1e-3 < dist) {
                continue;
            }
        }

        float cos_theta = dot(n_w, dir_l);

        // blinn-phong specular reflections
        float spec =
            shine > EPSILON ?
                pow(max(dot(normalize(dir_l - view_dir), n_w), 0.0), shine)
                : 0.0;

        // ignore normals at very close (<1) distances
        // change brightness (slightly) according to dominant access of normal
        float closeness = lerp(1.0, 0.0, clamp(dist, 0.0, 1.0));
        if (closeness > EPSILON) {
            CONST(float FACE_MULTIPLIERS[6]) = {
                0.9, 0.9, 1.0, 0.7, 0.8, 0.8
            };

            int da = dominant_axis(n_w);
            closeness *=
                FACE_MULTIPLIERS[(da * 2) + (n_w[da] < 0.0 ? 1 : 0)];
        }

        float t = closeness + max(cos_theta, 0.0);
        t = clamp(t, 0.0, 1.0);
        result +=
            (l.ambient.rgb
                + (l.diffuse.rgb * t)
                + (l.diffuse.rgb * spec))
            * att;
    }

    // dither, band to increments of 1/8
    result = dither(pos_w, result);
    result = round_multiple(result, 1.0f / 8.0f);
    return result;
}

void calculate(
    vec2 v_tc0,
    bool do_sun,
    float d,
    vec4 t_g,
    vec4 t_n,
    vec4 t_d,
    out vec4 color_bloom[2]) {
    uint flags = decode_u16(t_d.x);

    // TODO: experiment more with shine scaling factor
    float shine = t_n.a > EPSILON ? ((1.0f - t_n.a + EPSILON) * 64.0) : 0.0;

    vec3 color = t_g.rgb;
    vec3 pos_v =
        clip_to_view(
            u_look_light_invProj,
            clip_from_st_depth(v_tc0, d));
    vec3 pos_w = mul(u_look_light_invView, vec4(pos_v, 1.0));
    vec3 n_w = normalize((t_n.xyz * 2.0) - 1.0);

    if (d < 1.0 - EPSILON) {
        vec3 s =
            do_sun ?
                sunlight(u_look_light_direction.xyz, pos_w, n_w, shine)
                : 0.0f;
        vec3 m = light(u_look_light_direction.xyz, pos_w, n_w, shine);
        vec3 l = (color.rgb * (s + m));
        color_bloom[0] = vec4(vec3(l), t_g.a);

        float brightness = dot(l, vec3(0.2126, 0.7152, 0.0722));
        color_bloom[1] =
            vec4(
                brightness > 1.0 ? clamp(l, vec3(0.0), vec3(1.0)) : vec3(0.0),
                1.0);
    } else {
        color_bloom[0] = vec4(vec3(0.0), t_g.a);
        color_bloom[1] = vec4(vec3(0.0), 1.0);
    }
}

void main() {
    float
        d = texture2D(s_depth, v_texcoord0).r,
        d_tr = texture2D(s_depth_tr, v_texcoord0).r;
    vec4
        t_g = texture2D(s_gbuffer, v_texcoord0),
        t_g_tr = texture2D(s_gbuffer_tr, v_texcoord0),
        t_n = texture2D(s_normal, v_texcoord0),
        t_n_tr = texture2D(s_normal_tr, v_texcoord0),
        t_d = texture2D(s_data, v_texcoord0),
        t_d_tr = texture2D(s_data_tr, v_texcoord0);

    vec4 cb_base[2];
    vec4 cb_tr[2];

    calculate(v_texcoord0, true, d, t_g, t_n, t_d, cb_base);

    // manually depth test
    if (d_tr < d) {
        calculate(v_texcoord0, true, d_tr, t_g_tr, t_n_tr, t_d_tr, cb_tr);
    }

    // TODO: bloom for transparent objects? probably not necessary
    gl_FragData[0] = vec4(cb_base[0].rgb, 1.0);
    gl_FragData[1] = vec4(cb_base[1].rgb, 1.0);
    gl_FragData[2] = vec4(cb_tr[0].rgb, 1.0);
}
