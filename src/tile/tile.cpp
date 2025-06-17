#include "tile/tile.hpp"
#include "item/drop_table.hpp"
#include "level/level.hpp"
#include "level/light.hpp"
#include "entity/interaction.hpp"
#include "gfx/texture_atlas.hpp"

template<> TileRegistry &TileRegistry::instance() {
    static TileRegistry instance;
    return instance;
}

Tiles &Tiles::get() {
    static Tiles instance;
    return instance;
}

void Tiles::init() {
    TileRegistry::instance().initialize(*this);
}

LightArray Tile::lights(
    const Level &level,
    const ivec3 &pos) const {
    return LightArray();
}

DropTable Tile::drops(
    const Level &level,
    const ivec3 &pos) const {
    return DropTable::empty();
}


TextureArea Tile::texture(
    const Level *level,
    const ivec3 &pos,
    Direction::Enum dir) const {
    ASSERT(false);
    return TextureArea(nullptr, vec2(0), vec2(0), vec2(0), vec2(0));
}

InteractionAffordance Tile::can_interact_with_this(
    const Level &level,
    const ivec3 &pos,
    Direction::Enum face,
    InteractionKind kind,
    const Entity &e,
    ItemRef item) const {
    // TODO: YES but with some hardness() indicator
    return kind == InteractionKind::PRIMARY
        && this->destructible(level, pos) ?
        InteractionAffordance::YES
        : InteractionAffordance::NONE;
}

InteractionResult Tile::interacted(
    Level &level,
    const ivec3 &pos,
    Direction::Enum face,
    InteractionKind kind,
    Entity &e,
    ItemRef item) const {
    if (kind == InteractionKind::PRIMARY) {
        level.destroy_tile(pos, e, item);
        return InteractionResult::SUCCESS;
    }

    return InteractionResult::NONE;
}
