#include "tile/tile_stone.hpp"
#include "tile/tile_renderer_uniform.hpp"
#include "gfx/sprite_resource.hpp"
#include "gfx/specular_textures.hpp"
#include "item/drop_table.hpp"

static const auto spritesheet =
    SpritesheetResource("tile_stone", "tile/stone.png", { 16, 16 });

DropTable TileStone::drops(
    const Level &level,
    const ivec3 &pos) const {
    return { *this->item() };
}

const TileRenderer &TileStone::renderer() const {
    static auto renderer =
        TileRendererUniform(
            *this,
            spritesheet[uvec2(0, 0)],
            SpecularTextures::instance().min());
    return renderer;
}

const Item *TileStone::item() const {
    return &Items::get().get<ItemStone>();
}

DECL_TILE_REGISTER(TileStone)
DECL_ITEM_REGISTER(ItemStone)
