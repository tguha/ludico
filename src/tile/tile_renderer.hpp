#pragma once

#include "level/chunk_renderer.hpp"
#include "gfx/mesh_buffer.hpp"
#include "gfx/texture_atlas.hpp"

struct Tile;
struct Level;

struct TileRenderer {
    const Tile *tile;

    TileRenderer() = default;
    explicit TileRenderer(const Tile &tile);
    virtual ~TileRenderer() = default;

    // if true, responsibility for meshing is given to the chunk renderer (uses
    // default behavior)
    virtual bool is_default() const {
        return false;
    }

    // true if this tile has a custom mesh (need to call mesh() in order to
    // mesh it). used so that a tile can exist in a non-default pass but still
    // use default meshing behavior
    virtual bool custom() const {
        return true;
    }

    // which pass this mesh should be added to
    virtual ChunkRenderer::Pass pass() const {
        return ChunkRenderer::PASS_DEFAULT;
    }

    // meshes tile to dstination buffer
    // NOTE: level can be nullptr!
    virtual void mesh(
        const Level *level,
        const ivec3 &pos,
        MeshBuffer<ChunkVertex, u32> &dst) const = 0;

    // gets the tile's icon, if present
    virtual std::optional<TextureArea> icon() const;

    // gets the tile's colors, returning an empty span if none are present
    virtual std::span<const vec3> colors() const;

    // true if this tile has "extras" (need to call render_extras() during
    // rendering when in frustum)
    virtual bool has_extras() const {
        return false;
    }

    // TODO: re-doc
    // renders tile extras, only called if has_extras() is true
    virtual void render_extras(
        RenderContext &ctx,
        const Level *level,
        const ivec3 &pos) const {}

    // true if this tile's extras are instanced
    virtual bool has_instanced_extras() const {
        return false;
    }

    // TODO: re-doc
    // if has_instanced_extras(), called by level renderer after all chunk
    // rendering ONCE per tile renderer
    virtual void render_instanced_extras(
        RenderContext &ctx,
        const Level *level) const {}

    // render a tile by building its mesh buffer
    static void render(
        const Tile &tile,
        const Level *level,
        const ivec3 &pos,
        RenderState render_state,
        const vec4 &color = vec4(0.0f),
        f32 alpha = 1.0f);

    // renders a mesh buffer directly
    static void render(
        const Program &program,
        const mat4 &model,
        const MeshBuffer<ChunkVertex, u32> &buffer,
        RenderState render_state,
        const vec4 &color = vec4(0.0f));

    // load a tile, possibly using a custom mesh, into the specified buffer
    static void mesh_tile(
        const Tile &tile,
        const Level *level,
        const ivec3 &pos,
        MeshBuffer<ChunkVertex, u32> &dst);

    // load a default version of the specified tile to the meshbuffer
    static void mesh_default(
        const Tile &tile,
        const Level *level,
        const ivec3 &pos,
        MeshBuffer<ChunkVertex, u32> &dst);

    // mesh a tile as a specific subtile shape
    static void mesh_subtile(
        const Tile &tile,
        const Level *level,
        const ivec3 &pos,
        MeshBuffer<ChunkVertex, u32> &dst,
        u8 subtile);

private:
    mutable std::optional<TextureArea> _texture_area;
    mutable std::optional<std::vector<vec3>> _colors;
};
