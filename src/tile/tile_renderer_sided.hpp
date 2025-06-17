#pragma once

#include "tile/tile_renderer_basic.hpp"
#include "util/collections.hpp"

struct TileRendererSided : public TileRendererBasic {
    using Base = TileRendererBasic;

    // initializer_lists can be of size 1 (all the same), 3 (one per side pair),
    // or 6 (one for every side)
    explicit TileRendererSided(
        const Tile &tile,
        std::initializer_list<TextureArea> &&il_tex,
        std::initializer_list<TextureArea> &&il_specular)
        : Base(tile) {
        this->areas_tex =
            Direction::expand_directional_array<decltype(this->areas_tex)>(
                il_tex);
        this->areas_specular =
            Direction::expand_directional_array<decltype(this->areas_specular)>(
                il_specular);
    }

    TextureArea coords(
        const Level *level,
        const ivec3 &pos,
        Direction::Enum dir) const override {
        return this->areas_tex[dir];
    }

    TextureArea coords_specular(
        const Level *level,
        const ivec3 &pos,
        Direction::Enum dir) const override {
        return this->areas_specular[dir];
    }

private:
    std::array<TextureArea, Direction::COUNT> areas_tex, areas_specular;
};
