#pragma once

#include "tile/tile_renderer.hpp"
#include "voxel/voxel_model.hpp"

struct TileRendererVoxel : public TileRenderer {
    using Base = TileRenderer;
    using Base::Base;

    // offset into tile position at which to render model
    virtual vec3 offset() const {
        return vec3(0.0f);
    }

    virtual const VoxelModel &get_model(
        const Level *level,
        const ivec3 &pos) const = 0;

    void mesh(
        const Level *level,
        const ivec3 &pos,
        MeshBuffer<ChunkVertex, u32> &dest) const override;
};

struct TileRendererVoxelBasic : public TileRendererVoxel {
    TileRendererVoxelBasic() = default;
    explicit TileRendererVoxelBasic(
        const Tile &tile,
        VoxelModel &&model,
        const vec3 &offset = vec3(0.0))
        : TileRendererVoxel(tile),
           _model(std::move(model)),
           _offset(offset) {}

    vec3 offset() const override {
        return this->_offset;
    }

    const VoxelModel &get_model(
        const Level *level,
        const ivec3 &pos) const override {
        return this->_model;
    }

private:
    VoxelModel _model;
    vec3 _offset;
};
