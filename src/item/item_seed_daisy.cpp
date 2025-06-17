#include "level/level.hpp"
#include "item/item_seed_daisy.hpp"
#include "tile/tile_dirt.hpp"
#include "tile/tile_grass.hpp"
#include "entity/entity_daisy.hpp"
#include "entity/interaction.hpp"
#include "gfx/sprite_resource.hpp"

static const auto spritesheet =
    SpritesheetResource(
        "item_seed_daisy",
        "item/seed_daisy.png",
        { 8, 8 });

TextureArea ItemSeedDaisy::sprite() const {
    return spritesheet[uvec2(0, 0)];
}

EntitySpawnFn ItemSeedDaisy::spawn_fn(
    const ItemStack &item,
    const Entity &entity,
    const ivec3 &pos,
    Direction::Enum face) const {
    return [](Level &level, const vec3 &pos) {
        return level.spawn<EntityDaisy>(pos);
    };
}

InteractionAffordance ItemSeedDaisy::can_plant(
    const ItemStack &item,
    const Entity &entity,
    const ivec3 &pos,
    Direction::Enum face) const {
    const auto &level = item.level();
    const auto can =
        level.tiles[pos + Direction::to_ivec3(face)] == 0
            && tile::is_any_of<TileDirt, TileGrass>(
                Tiles::get()[level.tiles[pos]]);
    return can ? InteractionAffordance::YES : InteractionAffordance::NO;
}

DECL_ITEM_REGISTER(ItemSeedDaisy)
