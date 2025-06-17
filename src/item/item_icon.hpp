
#pragma once

#include "gfx/texture_atlas.hpp"
#include "gfx/sprite_batch.hpp"
#include "voxel/voxel_multi_mesh.hpp"

struct Item;
struct ItemStack;
struct Tile;
struct RenderContext;

struct ItemIcon {
    ItemIcon() = default;
    virtual ~ItemIcon() = default;

    void render_icon(
        const ItemStack *stack,
        SpriteBatch &batch,
        const ivec2 &pos,
        const vec4 &color = vec4(0.0f),
        isize z = SpriteBatch::Z_DEFAULT) const;

    virtual std::optional<TextureArea> texture_area(
        const ItemStack *stack) const {
        return std::nullopt;
    }

    virtual void render(
        RenderContext &ctx,
        const ItemStack *stack,
        const mat4 &model) const {}

    static VoxelMultiMesh &multi_mesh();
};

struct ItemIconMeshed : public ItemIcon {
    static const VoxelMultiMeshEntry &create_mesh(
        const TextureArea &area_tex,
        const TextureArea &area_spec);

    virtual const VoxelMultiMeshEntry &mesh(const ItemStack *stack) const = 0;

    void render(
        RenderContext &ctx,
        const ItemStack *stack,
        const mat4 &model) const override;
};

struct ItemIconBasic : public ItemIconMeshed {
    const VoxelMultiMeshEntry *_mesh;
    TextureArea area_tex, area_spec;

    ItemIconBasic() = default;
    explicit ItemIconBasic(
        const TextureArea &area_tex,
        std::optional<TextureArea> area_spec = std::nullopt);

    std::optional<TextureArea> texture_area(
        const ItemStack *stack) const override;

    const VoxelMultiMeshEntry &mesh(const ItemStack *stack) const override {
        return *this->_mesh;
    }
};

struct ItemIconTile : public ItemIcon {
    const Tile *tile;
    std::optional<TextureArea> area;

    ItemIconTile() = default;
    explicit ItemIconTile(const Tile &tile)
        : tile(&tile) {}

    std::optional<TextureArea> texture_area(
        const ItemStack *stack) const override;

    void render(
        RenderContext &ctx,
        const ItemStack *stack,
        const mat4 &model) const override;
};
