#pragma once

#include "tile/tile_renderer.hpp"

struct TileRendererSubtile : public TileRenderer {
    const Tile &tile;

    explicit TileRendererSubtile(const Tile &tile)
        : tile(tile) {}

    void mesh(
        const Level *level,
        const ivec3 &pos,
        MeshBuffer<ChunkVertex, u32> &dst) const override;
};
