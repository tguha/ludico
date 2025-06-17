#pragma once

#include "voxel/voxel_model.hpp"
#include "gfx/mesh.hpp"

struct VoxelMesh : public Mesh {
    VoxelModel model;

    VoxelMesh() = default;

    explicit VoxelMesh(const VoxelModel &model)
        : Mesh(model.mesh),
          model(model) {
            this->model.mesh = decltype(this->model.mesh)();
        }

    inline auto &shape() const {
        return this->model.shape;
    }

    inline auto &bounds() const {
        return this->model.bounds;
    }

    // returns the computed bounds size of this model, max - min
    inline auto size() const {
        return this->model.bounds.size();
    }

    inline auto shape_size() const {
        return vec3(this->model.shape.size);
    }
};
