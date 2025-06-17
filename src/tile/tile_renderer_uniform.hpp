#pragma once

#include "tile/tile_renderer_basic.hpp"

struct TileRendererUniform : public TileRendererBasic {
    using Base = TileRendererBasic;

    explicit TileRendererUniform(
        const Tile &tile,
        const TextureArea &area_tex,
        const TextureArea &area_specular)
        : Base(tile),
          area_tex(area_tex),
          area_specular(area_specular) {}

    TextureArea coords(
        const Level *level,
        const ivec3 &pos,
        Direction::Enum dir) const override {
        return area_tex;
    }

    TextureArea coords_specular(
        const Level *level,
        const ivec3 &pos,
        Direction::Enum dir) const override {
        return area_specular;
    }

private:
    TextureArea area_tex, area_specular;
};
