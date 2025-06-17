#pragma once

#include "gfx/multi_mesh.hpp"
#include "voxel/voxel_model.hpp"

struct VoxelMesh;
struct VoxelMultiMesh;

struct VoxelMultiMeshEntry : public TMultiMeshEntry<VoxelMultiMesh> {
    VoxelModel model;

    inline const AABB &bounds() const {
        return model.bounds;
    }

    inline const VoxelShape &shape() const {
        return model.shape;
    }

    inline vec3 size() const {
        return this->bounds().size();
    }
};

struct VoxelMultiMesh
    : public TMultiMesh<
        VertexTextureSpecularNormal,
        VoxelMultiMeshEntry> {
    using Base = TMultiMesh<
        VertexTextureSpecularNormal,
        VoxelMultiMeshEntry>;
    using Base::Base;

    const VoxelMultiMeshEntry &add(VoxelModel &&model);
};
