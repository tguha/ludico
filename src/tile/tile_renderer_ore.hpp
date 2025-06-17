#pragma once

#include "tile/tile_renderer_voxel.hpp"
#include "util/hash.hpp"

struct TileRendererOre : public TileRendererVoxel {
    using Base = TileRendererVoxel;

    std::array<VoxelModel, 8> models;

    TileRendererOre(
        const Tile &tile,
        const TextureArea &area_tex,
        const TextureArea &area_spec,
        u64 seed);

    const VoxelModel &get_model(
        const Level *level,
        const ivec3 &pos) const override {
        return this->models[hash(pos) % this->models.size()];
    }
};
