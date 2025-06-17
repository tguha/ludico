#include "level/chunk_renderer.hpp"
#include "level/chunk.hpp"
#include "level/level.hpp"
#include "tile/tile_renderer.hpp"
#include "tile/tile_renderer_basic.hpp"
#include "gfx/cube.hpp"
#include "gfx/program.hpp"
#include "gfx/renderer.hpp"
#include "gfx/mesh_buffer.hpp"
#include "gfx/render_context.hpp"
#include "constants.hpp"
#include "global.hpp"

DECL_VERTEX_TYPE(ChunkVertex) {
    ChunkVertex::layout
        .begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float, true)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord1, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, false, true)
        .end();
}

static void emit_face(
    ChunkRenderer &renderer,
    MeshBuffer<ChunkVertex, u32> &buffer,
    TileId id,
    ivec3 pos_c,
    vec3 position,
    TextureArea area_tex,
    TextureArea area_specular,
    Direction::Enum direction) {
    // index offset
    const usize offset = buffer.num_vertices();

    // emit vertices
    usize i_v = buffer.vertices.size();
    buffer.vertices.resize(buffer.vertices.size() + 4);
    for (usize i = 0; i < 4; i++) {
        ChunkVertex cv;
        const auto d =
            cube::VERTICES[
                cube::INDICES[
                    (direction * 6) + cube::UNIQUE_INDICES[i]]];
        cv.pos = position + d;
        cv.normal = cube::NORMALS[direction];
        cv.st =
            (cube::TEX_COORDS[i] * area_tex.sprite_unit)
                + area_tex.min;
        cv.st_specular =
            (cube::TEX_COORDS[i] * area_specular.sprite_unit)
                + area_specular.min;
        cv.tile_id = id;
        buffer.vertices[i_v++] = cv;
    }

    // emit indices
    usize i_i = buffer.indices.size();
    buffer.indices.resize(buffer.indices.size() + cube::FACE_INDICES.size());
    for (usize i : cube::FACE_INDICES) {
        buffer.indices[i_i++] = offset + i;
    }
}

static inline void emit_tile(
    ChunkRenderer &renderer,
    std::array<
        MeshBuffer<ChunkVertex, u32>,
        ChunkRenderer::PASS_COUNT> &buffers,
    const ivec3 &pos) {

    auto &chunk = renderer.chunk;
    const auto data = chunk[pos];

    const auto pos_w = pos + chunk.offset_tiles;
    const auto &t = Tiles::get()[Chunk::TileData::from(data)];

    const auto &tile_renderer = t.renderer();
    auto *buffer = &buffers[ChunkRenderer::PASS_DEFAULT];

    if (!tile_renderer.is_default()) {
        buffer = &buffers[tile_renderer.pass()];

        if (tile_renderer.custom()) {
            tile_renderer.mesh(
                chunk.level,
                pos,
                *buffer);
        }
        return;
    }

    // ghost tiles rendered in transparent buffer
    if (Chunk::GhostData::from(data)) {
        buffer = &buffers[ChunkRenderer::PASS_TRANSPARENT];
    }

    // TODO: consider a dynamic_cast here to catch bad things?
    const auto &renderer_basic =
        static_cast<const TileRendererBasic&>(tile_renderer);

    for (const auto &d : Direction::ALL) {
        const auto n = pos + Direction::to_ivec3(d);
        const auto proxy_n = chunk.or_level(n);
        const auto &t_n = Tiles::get()[Chunk::TileData::from(proxy_n)];

        if (proxy_n.present()
                && !Chunk::GhostData::from(proxy_n)
                && t_n.id != 0) {

            Tile::Transparency tt;
            if ((tt = t_n.transparency_type()) != Tile::Transparency::OFF) {
                if (tt == Tile::Transparency::MERGED && (t_n.id == t.id)) {
                    continue;
                }
            } else if (t.subtile()) {
                // only skip if full subtile
                if (Chunk::SubtileData::from(proxy_n) == 0xFF) {
                    continue;
                }
            } else {
                // neither transparent nor partial subtile, do not show
                continue;
            }
        }

        emit_face(
            renderer,
            *buffer,
            t.id,
            pos,
            vec3(pos),
            renderer_basic.coords(chunk.level, pos_w, d),
            renderer_basic.coords_specular(chunk.level, pos_w, d),
            d);
    }
}

ChunkRenderer::ChunkRenderer(Chunk &chunk)
    : chunk(chunk) {
    // TODO: why does this complain when the default size is small????
    // shouldn't they automatically resize >:(

    // TODO: pick a decent default size
    this->vertex_buffer =
        RDResource<bgfx::DynamicVertexBufferHandle>(
            bgfx::createDynamicVertexBuffer(
                4096 * 1,
                ChunkVertex::layout,
                BGFX_BUFFER_ALLOW_RESIZE),
            [](auto handle) { bgfx::destroy(handle); });

    // TODO: do we need 32 bit indices?
    this->index_buffer =
        RDResource<bgfx::DynamicIndexBufferHandle>(
            bgfx::createDynamicIndexBuffer(
                4096 * 1,
                BGFX_BUFFER_ALLOW_RESIZE
                | BGFX_BUFFER_INDEX32),
            [](auto handle) { bgfx::destroy(handle); });
}

void ChunkRenderer::mesh() {
    // one buffer per pass
    // NOTE: buffers are static! this way we can always keep the largest
    // buffers around and avoid re-allocating on expansion
    // TODO: super large buffers should (probably) be freed
    static std::array<
        MeshBuffer<ChunkVertex, u32>,
        PASS_COUNT> buffers;

    ivec3 pos;
    for (pos.x = 0; pos.x < Chunk::SIZE.x; pos.x++) {
        for (pos.y = 0; pos.y < Chunk::SIZE.y; pos.y++) {
            for (pos.z = 0; pos.z < Chunk::SIZE.z; pos.z++) {
                const TileId t = this->chunk[pos];
                if (t == 0) {
                    continue;
                }

                emit_tile(*this, buffers, pos);
            }
        }
    }

    // store indices for each pass, update buffer
    usize num_indices = 0, num_vertices = 0;

    MeshBuffer<ChunkVertex, u32> merged;

    // preallocate merged data, compute pass indices
    for (usize i = 0; i < buffers.size(); i++) {
        const auto &buffer = buffers[i];

        this->passes[i] = {
            std::make_tuple(num_indices, buffer.num_indices()),
            std::make_tuple(num_vertices, buffer.num_vertices()),
        };

        if (buffer.num_vertices() == 0) {
            ASSERT(buffer.num_indices() == 0);
            continue;
        }

        num_indices += buffer.num_indices();
        num_vertices += buffer.num_vertices();
    }

    merged.indices.reserve(num_indices);
    merged.vertices.reserve(num_vertices);

    // merge data
    for (auto &buffer : buffers) {
        merged.indices.insert(
            merged.indices.end(),
            buffer.indices.begin(),
            buffer.indices.end());
        merged.vertices.insert(
            merged.vertices.end(),
            buffer.vertices.begin(),
            buffer.vertices.end());

        // clear buffer
        // use resize(0) as it is guaranteed not to change capacity
        buffer.indices.resize(0);
        buffer.vertices.resize(0);
    }

    // TODO: eliminate this copy
    // upload
    if (num_indices != 0 && num_vertices != 0) {
        bgfx::update(
            this->index_buffer, 0,
            bgfx::copy(
                &merged.indices[0],
                merged.num_indices() * sizeof(merged.indices[0])));

        bgfx::update(
            this->vertex_buffer, 0,
            bgfx::copy(
                &merged.vertices[0],
                merged.num_vertices() * sizeof(merged.vertices[0])));
    }
}

static void render_pass(
    ChunkRenderer &chunk_renderer,
    ChunkRenderer::Pass pass,
    const mat4 &model,
    RenderState render_state,
    f64 alpha = 1.0) {
    const auto [offset_indices, num_indices] =
        chunk_renderer.passes[pass].indices_start_num;
    const auto [offset_vertices, num_vertices] =
        chunk_renderer.passes[pass].vertices_start_num;

    if (num_indices == 0) {
        return;
    }

    bgfx::setVertexBuffer(
        0, chunk_renderer.vertex_buffer, offset_vertices, num_vertices);
    bgfx::setIndexBuffer(
        chunk_renderer.index_buffer, offset_indices, num_indices);

    const auto &program = Renderer::get().programs["chunk"];
    program.set("s_tex", 0, TextureAtlas::get());
    program.try_set("u_color", vec4(0));
    program.try_set("u_alpha", vec4(alpha));
    program.try_set("u_flags_id", vec4(0));
    bgfx::setTransform(math::value_ptr(model));
    Renderer::get().submit(program, render_state.or_defaults());
}

void ChunkRenderer::render(RenderContext &ctx) {
    if (this->chunk.render_version == 0) {
        return;
    }

    // re-mesh if dirty
    if (this->chunk.render_version != this->mesh_version) {
        this->mesh();
        this->mesh_version = this->chunk.render_version;
    }

    auto *model = ctx.allocator->alloc<mat4>(mat4(1.0));
    *model = math::translate(*model, vec3(this->chunk.offset_tiles));

    ctx.push(
        RenderCtxFn {
            [this, model](const RenderGroup&, RenderState render_state) {
                if (render_state.flags & RENDER_FLAG_PASS_TRANSPARENT) {
                    render_pass(
                        *this, PASS_TRANSPARENT, *model, render_state, 0.5f);
                } else if (render_state.flags &
                            (RENDER_FLAG_PASS_BASE | RENDER_FLAG_PASS_SHADOW)) {
                    render_pass(
                        *this, PASS_DEFAULT, *model, render_state);
                }
            },
            RENDER_FLAG_PASS_BASE
                | RENDER_FLAG_PASS_SHADOW
                | RENDER_FLAG_PASS_TRANSPARENT
        });

}
