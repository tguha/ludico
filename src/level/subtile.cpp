#include "level/subtile.hpp"
#include "level/level.hpp"

void subtile::set(
    Level &level,
    const ivec3 &offset_tile,
    const ivec3 &offset_sub,
    const Tile &tile,
    bool value) {
    ASSERT(
        tile.subtile(),
        "Tile {} cannot be subtiled",
        tile.name());
    const auto pos_tile =
        offset_tile + (offset_sub / ivec3(subtile::SIZE));
    level.tiles[pos_tile] = tile;

    auto sub = level.subtile[pos_tile];

    u8 mask =
        1 << subtile::from_offset(uvec3(offset_sub) % subtile::SIZE);

    if (value) {
        sub = sub.get() | mask;
    } else {
        sub = sub.get() & ~mask;
    }
}

/* std::tuple<const Tile*, bool> subtile::get( */
/*     Level &level, */
/*     const ivec3 &offset_tile, */
/*     const ivec3 &offset_sub, */
/*     bool explicitly_subtile) { */
/*     auto proxy = level.raw[offset_tile + (offset_sub / ivec3(subtile::SIZE))]; */

/*     const auto &tile = Tiles::get()[Chunk::TileData::from(proxy)]; */
/*     if (explicitly_subtile && !tile.subtile()) { */
/*         return std::make_tuple(tile, false); */
/*     } */

/*     const auto sub = Chunk::SubtileData::from(proxy); */
/*     return std::make_tuple( */
/*         tile, */
/*         !!(sub & subtile::from_offset(uvec3(offset_sub) % subtile::SIZE))); */
/* } */
