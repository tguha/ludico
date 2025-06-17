#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#define TICKS_PER_SECOND (120)

// pixel scale
#define SCALE 16

// graphics configuration
#define SHADOW_SAMPLES 8
#define SSAO_SAMPLES 16
#define MAX_LIGHTS 1024

#define LIGHT_MAX_VALUE 15

// graphics flags
#define GFX_FLAG_NONE               0
#define GFX_FLAG_WATER              (1 << 0)
#define GFX_FLAG_NOEDGE             (1 << 1)
#define GFX_FLAG_NOLIGHT            (1 << 2)
#define GFX_FLAG_ENTITY_STENCIL     (1 << 3)

#define RENDER_FLAG_PASS_BASE           (1 << 0)
#define RENDER_FLAG_PASS_SHADOW         (1 << 1)
#define RENDER_FLAG_PASS_UI             (1 << 2)
#define RENDER_FLAG_PASS_TRANSPARENT    (1 << 3)
#define RENDER_FLAG_PASS_ENTITY_STENCIL (1 << 4)

// not a "real" pass - only used to mark pass which is first
#define RENDER_FLAG_PASS_FIRST          (1 << 5)

// not a "real" pass - only used to mark pass which is last
#define RENDER_FLAG_PASS_LAST           (1 << 6)

#define RENDER_FLAG_PASS_ALL_3D                                                \
    (RENDER_FLAG_PASS_BASE                                                     \
    | RENDER_FLAG_PASS_SHADOW                                                  \
    | RENDER_FLAG_PASS_TRANSPARENT                                             \
    | RENDER_FLAG_PASS_ENTITY_STENCIL)

#define RENDER_FLAG_PASS_ALL_2D                                                \
    (RENDER_FLAG_PASS_UI)

#define RENDER_FLAG_CVP                 (1 << 6)

// TODO: ???
#define BUFFER_INDEX____                1

#define BUFFER_INDEX_LIGHT              2
#define BUFFER_INDEX_PARTICLE           3
#define BUFFER_INDEX_SPRITE             4
#define BUFFER_INDEX_INSTANCE           5
#define BUFFER_INDEX_INSTANCE_DATA      6

// used to represent invalid indices which ought to generate VERTEX_INVALID in
// the vertex shader
#define INDEX_INVALID (65535)

#endif
