#include "tile/tile_sun_crystal.hpp"
#include "gfx/specular_textures.hpp"
#include "gfx/sprite_resource.hpp"

static const auto spritesheet =
    SpritesheetResource("tile_sun_crystal", "tile/sun_crystal.png", { 16, 16 });

TextureArea TileSunCrystal::coords_tex() const {
    return spritesheet[uvec2(0, 0)];
}

TextureArea TileSunCrystal::coords_specular() const {
    return SpecularTextures::instance().closest(0.8f);
}

DECL_TILE_REGISTER(TileSunCrystal);
