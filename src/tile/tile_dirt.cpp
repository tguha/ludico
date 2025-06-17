#include "tile/tile_dirt.hpp"
#include "tile/tile_renderer_uniform.hpp"
#include "gfx/sprite_resource.hpp"
#include "gfx/specular_textures.hpp"
#include "item/drop_table.hpp"

static const auto spritesheet =
    SpritesheetResource("tile_dirt", "tile/dirt.png", { 16, 16 });

DropTable TileDirt::drops(
    const Level &level,
    const ivec3 &pos) const {
    return { *this->item() };
}

const TileRenderer &TileDirt::renderer() const {
    static auto renderer =
        TileRendererUniform(
            *this,
            spritesheet[uvec2(0, 0)],
            SpecularTextures::instance().min());
    return renderer;
}

const Item *TileDirt::item() const {
    return &Items::get().get<ItemDirt>();
}

DECL_TILE_REGISTER(TileDirt)
DECL_ITEM_REGISTER(ItemDirt)
