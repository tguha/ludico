$input v_texcoord0, v_texcoord1, v_normal, v_color0, v_color1
// THIS MUST MATCH model/prelude_fs.sc !

#include "common.sc"
#include "entity_stencil.sc"

SAMPLER2D(s_tex, 0);

uniform vec4 u_render_flags;
uniform vec4 u_flags_id;

#define FS_MODEL_EXT_ARGS_EXTRA ,v_color0,v_color1

bool main_ext(
    inout vec4 color,
    inout float specular,
    inout vec4 flags_id,
    uint v_color0,
    uint v_color1) {
    flags_id.x = v_color0;
    flags_id.y = v_color1;
    return true;
}

#define FS_MODEL_PRELUDE
#define FS_MODEL_EXTENSION
#include "model/fs_model.sc"
