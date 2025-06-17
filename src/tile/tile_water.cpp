#include "tile/tile_water.hpp"
#include "tile/tile_renderer_uniform.hpp"
#include "gfx/sprite_resource.hpp"
#include "gfx/specular_textures.hpp"
#include "item/drop_table.hpp"

static const auto spritesheet =
    SpritesheetResource(
        "tile_water", "tile/water.png", { 16, 16 },
        [](const TextureAtlasEntry &entry) {
            entry.atlas->animate(
                entry,
                { 0, 0 },
                32,
                16,
                false);
        });

const TileRenderer &TileWater::renderer() const {
    static auto renderer =
        TileRendererUniform(
            *this,
            spritesheet[{ 0, 0 }],
            SpecularTextures::instance().min());
    return renderer;
}

DropTable TileWater::drops(
    const Level &level,
    const ivec3 &pos) const {
    return { *this->item() };
}

const Item *TileWater::item() const {
    return &Items::get().get<ItemWater>();
}

DECL_TILE_REGISTER(TileWater)
DECL_ITEM_REGISTER(ItemWater)
