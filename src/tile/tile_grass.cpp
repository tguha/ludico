#include "tile/tile_grass.hpp"
#include "tile/tile_dirt.hpp"
#include "tile/tile_renderer_sided.hpp"
#include "level/level.hpp"
#include "gfx/sprite_resource.hpp"
#include "gfx/specular_textures.hpp"
#include "gfx/multi_mesh_instancer.hpp"
#include "gfx/render_context.hpp"
#include "gfx/renderer.hpp"
#include "util/hash.hpp"
#include "util/rand.hpp"
#include "voxel/voxel_multi_mesh.hpp"
#include "item/drop_table.hpp"
#include "constants.hpp"

static const auto
    spritesheet_grass =
        SpritesheetResource("tile_grass", "tile/grass.png", { 16, 16 }),
    spritesheet_dirt =
        SpritesheetResource("tile_dirt", "tile/dirt.png", { 16, 16 });

struct TileRendererGrass : public TileRendererSided {
    using Base = TileRendererSided;

    VoxelMultiMesh mesh;
    std::vector<const VoxelMultiMeshEntry*> extras;

    mutable MultiMeshInstanceBuffer instance_buffer;
    mutable MultiMeshInstancer::RenderData instance_data;

    TileRendererGrass(const Tile &tile)
        : Base(
            tile,
            {
                spritesheet_grass[uvec2(1, 0)],
                spritesheet_grass[uvec2(1, 0)],
                spritesheet_grass[uvec2(1, 0)],
                spritesheet_grass[uvec2(1, 0)],
                spritesheet_grass[uvec2(0, 0)],
                spritesheet_dirt[uvec2(0, 0)],
            },
            {
                SpecularTextures::instance().min()
            }),
          mesh(&Renderer::get().allocator) {
        // 3x3 shapes to make, from bottom left to top right
        const auto shapes =
            types::make_array(
                // straight up
                types::make_array(1, 0, 0,1, 0, 0, 1, 0, 0),
                types::make_array(1, 0, 0, 1, 0, 0, 0, 0, 0),
                types::make_array(1, 0, 0, 0, 0, 0, 0, 0, 0),
                // L-shaped
                types::make_array(1, 1, 0, 1, 0, 0, 1, 0, 0),
                types::make_array(1, 1, 1, 1, 0, 0, 1, 0, 0),
                types::make_array(1, 1, 0, 1, 0, 0, 0, 0, 0),
                types::make_array(1, 1, 1, 1, 0, 0, 0, 0, 0),
                // little bits
                types::make_array(1, 0, 1, 0, 0, 0, 0, 0, 0),
                types::make_array(1, 0, 1, 1, 0, 0, 0, 0, 0),
                types::make_array(1, 0, 1, 0, 0, 1, 0, 0, 0),
                types::make_array(1, 0, 1, 1, 0, 0, 1, 0, 0),
                types::make_array(1, 0, 1, 0, 0, 1, 0, 0, 1));

        for (const auto &shape_data : shapes) {
            this->extras.push_back(
                &this->mesh.add(
                    VoxelModel::from_sprite(
                        *spritesheet_grass,
                        spritesheet_grass[{2, 0}],
                        SpecularTextures::instance().min(),
                        1,
                        VoxelModel::FLAG_MIN_BOUNDS,
                        [&](VoxelShape &shape) {
                            shape.clear();
                            shape.resize({ 3, 3, 1 });
                            shape.each(
                                [&](const ivec3 &pos) {
                                    shape[pos] =
                                        !!shape_data[(pos.x * 2) + pos.y];
                                });
                        })
                    )
                );
        }

        this->instance_buffer =
            MultiMeshInstanceBuffer(
                &global.frame_allocator,
                this->mesh);
    }

    bool has_extras() const override {
        return true;
    }

    void render_extras(
        RenderContext &ctx,
        const Level *level,
        const ivec3 &pos) const override {
        if (level && level->tiles[pos + ivec3(0, 1, 0)] != 0) {
            return;
        }

        auto rand = Rand(hash(pos));

        for (usize i = 0; i < rand.next<usize>(0, 4); i++) {
            const auto &mesh = *rand.pick(this->extras);
            const auto center_offset =
                math::xyz_to_xnz(mesh.bounds().size(), 0.0f) / 2.0f;

            // TODO: if this becomes too much of a performance drag a lot of the
            // more expensive matrices here can be precomputed and saved
            // manually compute normal matrix to save a costly
            // inverse-transpose :)
            auto m = mat4(1.0), n = mat4(1.0);
            m = math::translate(m, vec3(pos));
            m = math::translate(
                m,
                vec3(
                    rand.next(0.0f, 1.0f),
                    1.0f,
                    rand.next(0.0f, 1.0f)));
            m = math::scale(m, vec3(1.0f / SCALE));
            m = math::translate(m, +center_offset);
            const auto r =
                math::rotate(
                    mat4(1.0),
                    rand.next(0, 3) * math::PI_2,
                    vec3(0, 1, 0));
            m *= r;
            n *= r;
            m = math::translate(m, -center_offset);
            this->instance_buffer.push(
                ModelInstanceData {
                    .model = m,
                    .normal = mat3(n),
                    .flags = 0,
                    .id = 0
                },
                &mesh);
        }
    }

    bool has_instanced_extras() const override {
        return true;
    }

    void render_instanced_extras(
        RenderContext &ctx,
        const Level *level) const override {
        this->instance_data =
            Renderer::get().mm_instancer->push(this->instance_buffer);
        this->instance_buffer.clear();

        ctx.push(
            RenderCtxFn {
                [this](const RenderGroup&, RenderState render_state) {
                    Renderer::get().mm_instancer
                        ->render(
                            this->instance_data,
                            render_state.or_defaults(),
                            Renderer::get().programs["model_instanced"]);
                },
                RENDER_FLAG_PASS_ALL_3D
                    & ~RENDER_FLAG_PASS_TRANSPARENT
                    & ~RENDER_FLAG_PASS_ENTITY_STENCIL
            });
    }
};

DropTable TileGrass::drops(
    const Level &level,
    const ivec3 &pos) const {
    return { Items::get().get<ItemDirt>() };
}

const TileRenderer &TileGrass::renderer() const {
    static auto renderer = TileRendererGrass(*this);
    return renderer;
}

DECL_TILE_REGISTER(TileGrass)
