$input v_texcoord0

#include "common.sc"
#include "dither.sc"

SAMPLER2D(s_gbuffer, 0);
SAMPLER2D(s_normal, 1);
SAMPLER2D(s_depth, 2);
SAMPLER2D(s_sun, 3);
// SAMPLER2D(s_noise, 4);
SAMPLER2D(s_light, 5);
SAMPLER2D(s_light_tr, 6);
SAMPLER2D(s_bloom, 7);
SAMPLER2D(s_ssao, 8);
SAMPLER2D(s_edge, 9);
SAMPLER2D(s_data, 10);
SAMPLER3D(s_palette, 11);
SAMPLER2D(s_gbuffer_tr, 12);
SAMPLER2D(s_depth_tr, 13);

#define OCCM_DECL_UNIFORMS
SAMPLER3D(s_occm_full, 14);
SAMPLER2D(s_occm_top, 15);
SAMPLER3D(s_occm_blocking, 16);
#include "occlusion_map.sc"

SAMPLER2D(s_entity_stencil, 4);

DECL_CAMERA_UNIFORMS(u_look)

uniform vec4 u_highlight_colors[3];
uniform vec4 u_game_camera_pos;
uniform vec4 u_clear_color;

float get_top_occlusion(vec3 pos_w) {
    CONST(int) samples = 2;
    CONST(float) spacing = 0.25;
    CONST(float) offset = -float(samples / 2) * spacing;

    float result = 0.0;
    for (uint x = 0; x < samples; x++) {
        for (uint z = 0; z < samples; z++) {
            vec2 s = pos_w.xz;
            s += vec2(offset) + (vec2(x, z) * spacing);
            result += occm_sample_top(s) ? 1.0 : 0.0;
       }
    }

    return 1.0 - (result / float(samples * samples));
}

void main() {
    float d = texture2D(s_depth, v_texcoord0).r;
    vec4
        t_g = texture2D(s_gbuffer, v_texcoord0),
        t_n = texture2D(s_normal, v_texcoord0),
        t_d = texture2D(s_data, v_texcoord0);
    uint flags = decode_u16(t_d.r);
    vec3
        pos_v =
            clip_to_view(
                u_look_invProj,
                clip_from_st_depth(v_texcoord0, d)),
        pos_w = mul(u_look_invView, vec4(pos_v, 1.0)),
        n_w = normalize((t_n.xyz * 2.0) - 1.0);

    if (d < 1.0 - EPSILON) {
        vec3 light = texture2D(s_light, v_texcoord0).rgb;
        vec3 bloom = texture2D(s_bloom, v_texcoord0).rgb;
        vec4 edge = texture2D(s_edge, v_texcoord0);
        float o = texture2D(s_ssao, v_texcoord0).rgb;
        o = 1.0; // TODO: reenable SSAO
        vec3 c = clamp(o, 0.2, 1.0) * (light + bloom);

        if (!(flags & GFX_FLAG_NOEDGE)) {
            c *= clamp(1.0 - edge.a, 0.6, 1.0);
        }

        // entity highlights
        for (uint i = 0; i < 3; i++) {
            c = mix(
                c,
                u_highlight_colors[i].rgb,
                edge[i] * u_highlight_colors[i].a);
        }

        // clamp so that nothing ever goes to 1.0, this causes palette sampling
        // inaccuracies (whites become yellow, etc.)
        c = clamp(c, vec3(0.0), vec3(0.999));

        // palettize
        // c = texture3D(s_palette, c).rgb;

        // blend transparent samples over
        vec4 t_g_tr = texture2D(s_gbuffer_tr, v_texcoord0);
        if (t_g_tr.a > EPSILON) {
            // manually depth test
            float d_tr = texture2D(s_depth_tr, v_texcoord0);
            if (d_tr < d) {
                vec3 light_tr = texture2D(s_light_tr, v_texcoord0).rgb;
                light_tr = clamp(light_tr, vec3(0.0), vec3(0.999999));
                vec3 pal_tr = texture3D(s_palette, light_tr);
                c = mix(c, pal_tr, t_g_tr.a);
            }
        }

        c *= dither(pos_w, vec3(get_top_occlusion(pos_w)));

        gl_FragColor = vec4(c, 1.0);
    } else {
        gl_FragColor = vec4(vec3(u_clear_color.rgb), 1.0);
    }

    const uint show_buffer = 0;
    if (show_buffer == 1) {
        gl_FragColor = vec4(texture2D(s_depth, v_texcoord0).rrr, 1.0);
    } else if (show_buffer == 2) {
        gl_FragColor = vec4(texture2D(s_gbuffer, v_texcoord0).rgb, 1.0);
    } else if (show_buffer == 3) {
        gl_FragColor = vec4(texture2D(s_normal, v_texcoord0).rgb, 1.0);
    } else if (show_buffer == 4) {
        gl_FragColor = vec4(texture2D(s_sun, v_texcoord0).rgb, 1.0);
    } else if (show_buffer == 5) {
        gl_FragColor = vec4(texture2D(s_light, v_texcoord0).rgb, 1.0);
    } else if (show_buffer == 6) {
        gl_FragColor = vec4(texture2D(s_bloom, v_texcoord0).rgb, 1.0);
    } else if (show_buffer == 7) {
        float o = texture2D(s_ssao, v_texcoord0).r;
        o -= mod(o, 1.0 / 8.0);
        gl_FragColor = vec4(vec3(o), 1.0);
    } else if (show_buffer == 8) {
        gl_FragColor = vec4(texture2D(s_edge, v_texcoord0).rgb, 1.0);
    } else if (show_buffer == 9) {
        uint shine = (decode_u8(t_n.a) >> 4) & 0xF;
        float lightmap = t_d.r;
        gl_FragColor = vec4(
            vec2(
                lightmap / 16.0f,
                shine / 16.0f
            ), 0.0, 1.0);
    } else if (show_buffer == 10) {
        uint tile_id = decode_u16(texture2D(s_data, v_texcoord0).a);
        gl_FragColor = vec4(vec3(mod(tile_id / 64.0, 1.0)), 1.0);
    } else if (show_buffer == 11) {
        gl_FragColor = vec4(pos_w / 32.0, 1.0);
    }  else if (show_buffer == 12) {
        gl_FragColor = vec4(vec3(t_n.a), 1.0);
    }
}
