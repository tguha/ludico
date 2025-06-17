#pragma once

#include "tile/tile_renderer.hpp"
#include "util/direction.hpp"

struct TileRendererBasic : public TileRenderer {
    using Base = TileRenderer;
    using Base::Base;

    bool is_default() const override {
        return true;
    }

    bool custom() const override {
        return false;
    }

    void mesh(
        const Level *level,
        const ivec3 &pos,
        MeshBuffer<ChunkVertex, u32> &dst) const override;

    virtual TextureArea coords(
        const Level *level,
        const ivec3 &pos,
        Direction::Enum dir) const = 0;

    virtual TextureArea coords_specular(
        const Level *level,
        const ivec3 &pos,
        Direction::Enum dir) const = 0;
};
