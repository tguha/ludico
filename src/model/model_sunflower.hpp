#pragma once

#include "model/model_flower_multi.hpp"

struct ModelSunflower : public ModelFlowerMulti {
    using Base = ModelFlowerMulti;

    ModelSunflower();

    std::span<const vec3> colors() const override;

    const VoxelMultiMeshEntry &get_mesh_stem(
        const EntityPlant &plant,
        usize i = 0) const override;
};
