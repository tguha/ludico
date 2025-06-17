#pragma once

#include "tile/tile_renderer.hpp"
#include "util/direction.hpp"

struct TileRendererSized : public TileRenderer {
    using Base = TileRenderer;
    using Base::Base;

    void mesh(
        const Level *level,
        const ivec3 &pos,
        MeshBuffer<ChunkVertex, u32> &dst) const override;

    virtual vec3 size() const {
        return vec3(1.0f);
    }

    virtual TextureArea coords(
        const Level *level,
        const ivec3 &pos,
        Direction::Enum dir) const = 0;

    virtual TextureArea coords_specular(
        const Level *level,
        const ivec3 &pos,
        Direction::Enum dir) const = 0;
};
