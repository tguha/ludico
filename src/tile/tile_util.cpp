#include "tile/tile.hpp"

const Tile &tile::detail::lookup(TileId i) {
    return Tiles::get()[i];
}

const Tile &tile::detail::lookup(std::type_index type_index) {
    return Tiles::get().get(type_index);
}
