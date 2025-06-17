#pragma once

#include "model/model_flower_multi.hpp"

struct ModelHydrangea : public ModelFlowerMulti {
    using Base = ModelFlowerMulti;

    ModelHydrangea();

    const VoxelMultiMeshEntry &get_mesh_flower(
        const EntityPlant &plant,
        usize i = 0) const override;

    usize n_flowers(const EntityPlant &plant) const override;

    vec3 offset(const EntityPlant &plant, usize i) const override;

    std::span<const vec3> colors() const override;
};
