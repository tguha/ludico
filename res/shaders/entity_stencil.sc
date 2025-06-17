#ifndef ENTITY_STENCIL_SC
#define ENTITY_STENCIL_SC
#ifndef __cplusplus
#define IS_ENTITY_STENCIL_PASS()                                               \
    ((uint(u_render_flags.x) & RENDER_FLAG_PASS_ENTITY_STENCIL) != 0)

#define SHOULD_STENCIL_ENTITY()                                                \
    ((uint(u_flags_id.x) & GFX_FLAG_ENTITY_STENCIL) != 0)

#define WRITE_ENTITY_STENCIL() gl_FragData[0] = u_flags_id.y;
#endif // ifndef __cplusplus
#endif // ifndef ENTITY_STENCIL_SC
