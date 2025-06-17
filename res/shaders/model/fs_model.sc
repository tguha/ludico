#include "model/prelude_fs.sc"

// use FS_MODEL_EXT_ARGS_EXTRA to add arguments to be passed to main_ext
#ifndef FS_MODEL_EXT_ARGS_EXTRA
#define FS_MODEL_EXT_ARGS_EXTRA
#endif

void main() {
    if (all(equal(v_texcoord0, VERTEX_INVALID_VEC4))) {
        discard;
    }

    vec4 color = texture2D(s_tex, v_texcoord0);

    if (color.a < EPSILON) {
        discard;
    }

    if (IS_ENTITY_STENCIL_PASS()) {
        if (!SHOULD_STENCIL_ENTITY()) {
            discard;
        }

        WRITE_ENTITY_STENCIL();
        return;
    }

    float specular = texture2D(s_tex, v_texcoord1).r;
    vec4 flags_id = u_flags_id;

#ifdef FS_MODEL_EXTENSION
    if (!main_ext(color, specular, flags_id FS_MODEL_EXT_ARGS_EXTRA)
            || color.a < EPSILON) { discard; }
#endif

    WRITE_GBUFFERS(
        vec4(color.rgb, 1.0),
        v_normal,
        specular,
        uint(flags_id.x),
        uint(flags_id.y),
        0);
}
