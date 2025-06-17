#include "tile/tile_ore.hpp"
#include "item/drop_table.hpp"
#include "gfx/specular_textures.hpp"
#include "gfx/sprite_resource.hpp"

static const auto
    spritesheet_copper =
        SpritesheetResource("tile_copper", "tile/copper.png", { 16, 16 }),
    spritesheet_iron =
        SpritesheetResource("tile_iron", "tile/iron.png", { 16, 16 }),
    spritesheet_silver =
        SpritesheetResource("tile_silver", "tile/silver.png", { 16, 16 }),
    spritesheet_gold =
        SpritesheetResource("tile_gold", "tile/gold.png", { 16, 16 });

DropTable TileOre::drops(const Level&, const ivec3&) const {
    return DropTable::empty();
}

TextureArea TileOreCopper::coords_tex() const {
    return spritesheet_copper[uvec2(0, 0)];
}

TextureArea TileOreCopper::coords_specular() const {
    return SpecularTextures::instance().closest(0.5f);
}

TextureArea TileOreIron::coords_tex() const {
    return spritesheet_iron[uvec2(0, 0)];
}

TextureArea TileOreIron::coords_specular() const {
    return SpecularTextures::instance().closest(0.1f);
}

TextureArea TileOreSilver::coords_tex() const {
    return spritesheet_silver[uvec2(0, 0)];
}

TextureArea TileOreSilver::coords_specular() const {
    return SpecularTextures::instance().closest(0.6f);
}

TextureArea TileOreGold::coords_tex() const {
    return spritesheet_gold[uvec2(0, 0)];
}

TextureArea TileOreGold::coords_specular() const {
    return SpecularTextures::instance().closest(0.7f);
}

DECL_TILE_REGISTER(TileOreCopper)
DECL_TILE_REGISTER(TileOreIron)
DECL_TILE_REGISTER(TileOreSilver)
DECL_TILE_REGISTER(TileOreGold)
