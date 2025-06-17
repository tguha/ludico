#pragma once

#include "model/model_flower.hpp"
#include "voxel/voxel_multi_mesh.hpp"

struct EntityPlant;

struct ModelFlowerMulti : public ModelFlower {
    using Base = ModelFlower;

    VoxelMultiMesh mesh;

    // NOTE: all mesh vectors must be initialized in subclasses

    // meshes for each flower stage
    std::vector<const VoxelMultiMeshEntry*> meshes_flower;

    // possible leaf meshes
    std::vector<const VoxelMultiMeshEntry*> meshes_leaf;

    // stem meshes
    std::vector<const VoxelMultiMeshEntry*> meshes_stem;

    // meshes in small growth stages
    std::vector<std::vector<const VoxelMultiMeshEntry*>> meshes_small;

    ModelFlowerMulti();

    // number of flowers for entity
    virtual usize n_flowers(const EntityPlant &entity) const {
        return 1;
    }

    // get offset of the ith flower from center
    virtual vec3 offset(const EntityPlant &plant, usize i) const {
        return vec3(0);
    }

    bool use_small_mesh(
        const EntityPlant &plant,
        usize i = 0) const override;

    const VoxelMultiMeshEntry &get_mesh_small(
        const EntityPlant &plant,
        usize i = 0) const override;

    const VoxelMultiMeshEntry &get_mesh_stem(
        const EntityPlant &plant,
        usize i = 0) const override;

    const VoxelMultiMeshEntry &get_mesh_flower(
        const EntityPlant &plant,
        usize i = 0) const override;

     void get_mesh_leaves(
        List<const VoxelMultiMeshEntry*> &dst,
        const EntityPlant &plant,
        usize i = 0) const override;

    bool render(
        RenderContext &ctx,
        const EntityPlant &plant) const;
};
