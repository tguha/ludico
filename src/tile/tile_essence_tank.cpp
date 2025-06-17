#include "tile/tile_essence_tank.hpp"
#include "tile/tile_renderer_sized.hpp"
#include "gfx/sprite_resource.hpp"
#include "gfx/specular_textures.hpp"
#include "item/drop_table.hpp"

static const auto spritesheet =
    SpritesheetResource(
        "tile_essence_tank", "tile/essence_tank.png", { 16, 16 });

DropTable TileEssenceTank::drops(
    const Level &level,
    const ivec3 &pos) const {
    return { *this->item() };
}

const TileRenderer &TileEssenceTank::renderer() const {
    struct TileRendererEssenceTank : TileRendererSized {
        using Base = TileRendererSized;
        using Base::Base;

        vec3 size() const override {
            return vec3(12.0f, 14.0f, 12.0f) / SCALE;
        }

        TextureArea coords(
            const Level *level,
            const ivec3 &pos,
            Direction::Enum dir) const override {
            return
                (dir == Direction::TOP || dir == Direction::BOTTOM) ?
                spritesheet[{ 0, 0 }]
                    .subpixels(AABB2u({ 0, 0 }, { 11, 11 }))
                : spritesheet[{ 1, 0 }]
                    .subpixels(AABB2u({ 0, 0 }, { 11, 13 }));
        }

        TextureArea coords_specular(
            const Level *level,
            const ivec3 &pos,
            Direction::Enum dir) const override {
            return SpecularTextures::instance().closest(0.8f);
        }
    };

    static auto renderer = TileRendererEssenceTank(*this);
    return renderer;
}

const Item *TileEssenceTank::item() const {
    return &Items::get().get<ItemEssenceTank>();
}

DECL_TILE_REGISTER(TileEssenceTank)
DECL_ITEM_REGISTER(ItemEssenceTank)
