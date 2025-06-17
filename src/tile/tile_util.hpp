#pragma once

#include "util/types.hpp"

struct Tile;

using TileId = u16;

static constexpr auto MAX_TILES = 4096;

namespace tile {
    namespace detail {
        const Tile &lookup(TileId i);

        const Tile &lookup(std::type_index type_index);
    }

    template <typename ...Ts>
    static inline bool is_any_of(const auto &tile) {
        const Tile *t;
        if constexpr (std::is_same_v<std::decay_t<decltype(tile)>, const Tile>) {
             t = &tile;
        } else {
            TileId i = tile;
            t = &tile::detail::lookup(i);
        }

        std::array<const Tile*, sizeof...(Ts)> ts = {
            &tile::detail::lookup(std::type_index(typeid(Ts)))...
        };

        return std::any_of(
            ts.begin(),
            ts.end(),
            [&](const auto *u) {
                return u == t;
            });
    }
}
