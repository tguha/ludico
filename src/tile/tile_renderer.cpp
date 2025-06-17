#include "tile/tile_renderer.hpp"
#include "tile/tile_renderer_basic.hpp"
#include "voxel/voxel_model.hpp"
#include "level/subtile.hpp"
#include "gfx/icon_renderer.hpp"
#include "gfx/renderer.hpp"
#include "gfx/cube.hpp"
#include "gfx/pixels.hpp"
#include "util/camera.hpp"
#include "util/collections.hpp"
#include "util/optional.hpp"
#include "constants.hpp"
#include "global.hpp"

// subtile meshes for every possible variant
// NOTE: first buffer is empty!
static std::array<MeshBuffer<ChunkVertex, u32>, 256> subtile_meshes;
static bool subtile_initialized = false;

// tile icon label prefix
constexpr auto LABEL_PREFIX = "tile_icon_";

static void render_icon(
    TileId id,
    RenderState render_state) {
    Camera camera;
    const auto d = math::sqrt(1.0f / 1.5f);
    camera.proj = math::ortho(-d, d, -d, d, -16.0f, 16.0f);
    camera.view = math::lookAt(
        vec3(math::sqrt(1.0f / 3.0f)),
        vec3(0.0),
        vec3(0, 1, 0));
    camera.update();
    camera.set_view_transform(render_state.view);

    MeshBuffer<ChunkVertex, u32> dest;
    TileRenderer::mesh_tile(
        Tiles::get()[id],
        nullptr,
        ivec3(0),
        dest);

    const auto &program = Renderer::get().programs["chunk_icon"];
    program.try_set("u_flags_id", vec4(0));
    TileRenderer::render(
        program,
        mat4(1.0),
        dest,
        render_state);
}

TileRenderer::TileRenderer(const Tile &tile)
    : tile(&tile) {
    Renderer::get().icon_renderer->add(
        LABEL_PREFIX + tile.name(),
        uvec2(10),
        [id = tile.id](RenderState render_state) {
            render_icon(id, render_state);
        });
}

std::optional<TextureArea> TileRenderer::icon() const {
    if (!this->_texture_area) {
        const auto entry =
            Renderer::get().icon_renderer->get(
                LABEL_PREFIX + this->tile->name());

        this->_texture_area =
            entry ?
                std::make_optional<TextureArea>((**entry)[{0, 0}])
                : std::nullopt;
    }

    return this->_texture_area;
}

std::span<const vec3> TileRenderer::colors() const {
    if (!this->_colors) {
        const auto area = OPT_OR_RET(this->icon(), std::span<const vec3>());
        this->_colors =
            collections::move_to<std::vector<vec3>>(
                pixels::colors(Pixels(area)),
                [](const vec4 &v) { return v.rgb(); });
    }

    return this->_colors ? std::span(*this->_colors) : std::span<const vec3>();
}

void TileRenderer::render(
    const Tile &tile,
    const Level *level,
    const ivec3 &pos,
    RenderState render_state,
    const vec4 &color,
    f32 alpha) {
    MeshBuffer<ChunkVertex, u32> buffer;
    TileRenderer::mesh_tile(
        tile,
        nullptr,
        ivec3(0),
        buffer);

    const auto &program = Renderer::get().programs["chunk"];
    program.set("u_alpha", vec4(alpha));
    TileRenderer::render(
        program,
        math::translate(mat4(1.0), vec3(pos)),
        buffer,
        render_state);
}

void TileRenderer::render(
    const Program &program,
    const mat4 &model,
    const MeshBuffer<ChunkVertex, u32> &buffer,
    RenderState render_state,
    const vec4 &color) {
    program.try_set("s_tex", 0, TextureAtlas::get());
    program.try_set("u_color", color);
    Renderer::get().render_buffers(
        program, model, ChunkVertex::layout,
        std::span(std::as_const(buffer.vertices)),
        std::span(std::as_const(buffer.indices)),
        render_state.or_defaults());
}

void TileRenderer::mesh_tile(
    const Tile &tile,
    const Level *level,
    const ivec3 &pos,
    MeshBuffer<ChunkVertex, u32> &dst) {
    const auto &renderer = tile.renderer();
    if (renderer.custom()) {
        renderer.mesh(level, pos, dst);
    } else {
        TileRenderer::mesh_default(
            tile, level, pos, dst);
    }
}

void TileRenderer::mesh_default(
    const Tile &tile,
    const Level *level,
    const ivec3 &pos,
    MeshBuffer<ChunkVertex, u32> &dst) {
    const auto &renderer_basic =
        static_cast<const TileRendererBasic&>(tile.renderer());

    for (const auto &d : Direction::ALL) {
        const auto offset = dst.num_vertices();
        const auto
            area_tex = renderer_basic.coords(level, pos, d),
            area_specular = renderer_basic.coords_specular(level, pos, d);

        for (usize i = 0; i < 4; i++) {
            const auto p =
                cube::VERTICES[
                    cube::INDICES[
                        (usize(d) * 6) + cube::UNIQUE_INDICES[i]]];
            ChunkVertex vertex;
            vertex.pos = vec3(pos) + p;
            vertex.normal = cube::NORMALS[d];
            vertex.st =
                (cube::TEX_COORDS[i] * area_tex.sprite_unit)
                    + area_tex.min;
            vertex.st_specular =
                (cube::TEX_COORDS[i] * area_specular.sprite_unit)
                    + area_specular.min;
            vertex.tile_id = tile.id;
            dst.vertices.push_back(vertex);
        }

        for (usize i : cube::FACE_INDICES) {
            dst.indices.push_back(offset + i);
        }
    }
}

// generates a subtile mesh (no texture coordinates/extra info/etc.) for the
// specified subtile layout. uses VoxelModel as backend.
static MeshBuffer<ChunkVertex, u32> subtile_generate(u8 subtiles) {
    // generate shape from present subtiles
    VoxelShape shape(subtile::SIZE);
    for (usize i = 0; i < subtile::COUNT; i++) {
        const auto offset =
            subtile::to_offset(
                static_cast<subtile::Enum>(i));
        shape[offset] = !!(subtiles & (1 << i));
    }

    const auto texel = TextureAtlas::get().texel();
    const auto texcoords = AABB2(vec2(0), texel * vec2(SCALE));
    const auto model =
        VoxelModel::make(
            shape,
            texel * vec2(SCALE) / vec2(subtile::SIZE.xy()),
            Direction::expand_directional_array<
                std::array<std::optional<AABB2>, Direction::COUNT>>(
                    std::make_optional(texcoords)),
            Direction::expand_directional_array<
                std::array<std::optional<AABB2>, Direction::COUNT>>(
                    std::make_optional(texcoords)),
            {});

    // convert buffer into chunk vertices
    MeshBuffer<ChunkVertex, u32> result;

    for (const auto &v : model.mesh.vertices) {
        ChunkVertex c;
        c.pos = v.position / vec3(subtile::SIZE);
        c.normal = v.normal;
        c.st = v.st;
        result.vertices.push_back(c);
    }

    std::copy(
        model.mesh.indices.begin(), model.mesh.indices.end(),
        std::back_inserter(result.indices));

    return result;
}

static void subtile_init() {
    for (usize i = 0; i < 1 << subtile::COUNT; i++) {
        subtile_meshes[i] = subtile_generate(i);
    }

    subtile_initialized = true;
}

static const MeshBuffer<ChunkVertex, u32> &subtile_get_or_create(u8 subtiles) {
    if (!subtile_initialized) {
        subtile_init();
    }

    return subtile_meshes[subtiles];
}

void TileRenderer::mesh_subtile(
    const Tile &tile,
    const Level *level,
    const ivec3 &pos,
    MeshBuffer<ChunkVertex, u32> &dst,
    u8 subtile) {

    const auto &renderer_basic =
        static_cast<const TileRendererBasic&>(tile.renderer());

    const auto offset = dst.num_vertices();
    const auto &buffer = subtile_get_or_create(subtile);
    for (const auto &v : buffer.vertices) {
        ChunkVertex c = v;
        const auto d = Direction::from_vec3(c.normal);
        c.st += renderer_basic.coords(level, pos, d).min;
        c.st_specular += renderer_basic.coords_specular(level, pos, d).min;
        c.tile_id = tile.id;
        c.pos += vec3(pos);
        dst.vertices.push_back(c);
    }

    for (const auto &i : buffer.indices) {
        dst.indices.push_back(offset + i);
    }
}
