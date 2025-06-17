#ifndef FS_MODEL_PRELUDE
$input v_texcoord0, v_texcoord1, v_normal
#include "common.sc"
#include "entity_stencil.sc"

SAMPLER2D(s_tex, 0);

uniform vec4 u_render_flags;
uniform vec4 u_flags_id;
#define FS_MODEL_PRELUDE
#endif
