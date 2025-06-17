#pragma once

#include "util/types.hpp"
#include "util/util.hpp"
#include "util/math.hpp"
#include "gfx/vertex.hpp"
#include "gfx/util.hpp"

struct Chunk;
struct RenderContext;

struct ChunkVertex : public VertexType<ChunkVertex> {
    static bgfx::VertexLayout layout;

    vec3 pos;
    vec3 normal;
    vec2 st;
    vec2 st_specular;

    // ideally would be u16 BUT Vulkan under Metal requires that vertex strides
    // are aligned to 4 bytes
    u32 tile_id;

    ChunkVertex() = default;
} PACKED;

DECL_VERTEX_TYPE_HEADER(ChunkVertex)

struct ChunkRenderer {
    // chunk mesh passes, used to separate out different parts of chunk meshes
    // if necessary
    enum Pass {
        PASS_DEFAULT = 0,
        PASS_TRANSPARENT = 1,
        PASS_COUNT = PASS_TRANSPARENT + 1
    };

    Chunk &chunk;

    // buffer indices for each pass
    struct {
        std::tuple<usize, usize> indices_start_num, vertices_start_num;
    } passes[PASS_COUNT];

    // version of the chunk (Chunk::version) when it was last meshed
    usize mesh_version = 0;

    RDResource<bgfx::DynamicIndexBufferHandle> index_buffer;
    RDResource<bgfx::DynamicVertexBufferHandle> vertex_buffer;

    explicit ChunkRenderer(Chunk &chunk);

    void mesh();
    void render(RenderContext &ctx);
};
