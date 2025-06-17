#include "item/item_icon.hpp"
#include "gfx/icon_renderer.hpp"
#include "gfx/renderer_resource.hpp"
#include "gfx/specular_textures.hpp"
#include "gfx/sprite_batch.hpp"
#include "gfx/render_context.hpp"
#include "gfx/renderer.hpp"
#include "voxel/voxel_multi_mesh.hpp"
#include "level/chunk_renderer.hpp"
#include "tile/tile.hpp"
#include "tile/tile_renderer.hpp"
#include "constants.hpp"
#include "global.hpp"

static auto meshes =
    RendererResource<VoxelMultiMesh>(
        []() { return VoxelMultiMesh(&Renderer::get().allocator); });

VoxelMultiMesh &ItemIcon::multi_mesh() {
    return *meshes;
}

void ItemIcon::render_icon(
    const ItemStack *stack,
    SpriteBatch &batch,
    const ivec2 &pos,
    const vec4 &color,
    isize z) const {
    if (auto area = this->texture_area(stack)) {
        batch.push(*area, pos, color, 1.0f, z);
    }
}

const VoxelMultiMeshEntry &ItemIconMeshed::create_mesh(
    const TextureArea &area_tex,
    const TextureArea &area_spec) {
    return
        meshes->add(
            VoxelModel::from_sprite(
                *area_tex.parent,
                area_tex,
                area_spec,
                1,
                VoxelModel::FLAG_MIN_BOUNDS));
}

void ItemIconMeshed::render(
    RenderContext &ctx,
    const ItemStack *stack,
    const mat4 &model) const {
   ctx.push(
        ModelPart {
            .entry = &this->mesh(stack),
            .model = math::scale(model, 1.0f / vec3(SCALE)),
            .passes = RENDER_FLAG_PASS_ALL_3D & ~RENDER_FLAG_PASS_TRANSPARENT
        });
}

ItemIconBasic::ItemIconBasic(
    const TextureArea &area_tex,
    std::optional<TextureArea> area_spec)
    : area_tex(area_tex),
      area_spec(
          area_spec ? *area_spec : SpecularTextures::instance().min()) {
    this->_mesh =
        &ItemIconMeshed::create_mesh(this->area_tex, this->area_spec);
}

std::optional<TextureArea> ItemIconBasic::texture_area(
    const ItemStack *stack) const {
    return this->area_tex;
}

std::optional<TextureArea> ItemIconTile::texture_area(
    const ItemStack *stack) const {
    return this->tile->renderer().icon();
}

// TODO: move into tile_renderer as a static function?
void ItemIconTile::render(
    RenderContext &ctx,
    const ItemStack *stack,
    const mat4 &model) const {

    auto *buffer = ctx.allocator->alloc<MeshBuffer<ChunkVertex, u32>>();
    TileRenderer::mesh_tile(
        *this->tile,
        nullptr,
        ivec3(0),
        *buffer);

    // NOTE: breaks on u_cvp* because same uniform is used across different draw
    // calls - how do we properly track this? ... maybe wait to apply uniforms
    // until submit() is called and have program keep track of its most recently
    // applied uniforms...
    // but that seems like a workaround?
    // could also try to avoid group.uniforms.try_apply here, but that may
    // involve moving all tile item icons into a separate rendering group -
    // which may also make sense, given that they use an entirely differnt
    // program...
    auto &group_old = ctx.top_group();
    auto &group = ctx.push_group();
    {
        group.inherit(group_old);
        group.group_flags &= ~RenderGroup::INSTANCED;
        group.uniforms.set("u_color", vec4(0.0));
        group.program = &Renderer::get().programs["chunk"];

        auto m = ctx.allocator->alloc<mat4>(model);
        *m = math::scale(*m, vec3(0.4f));
        ctx.push(
            RenderCtxFn {
                [buffer, m](
                    const RenderGroup &group,
                    RenderState render_state) {
                    Renderer::get().render_buffers(
                        *group.program,
                        *m,
                        ChunkVertex::layout,
                        std::span(std::as_const(buffer->vertices)),
                        std::span(std::as_const(buffer->indices)),
                        render_state.or_defaults());
                }});
    }
    ctx.pop_group();
}
