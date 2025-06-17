#pragma once

#include "model/model_entity.hpp"
#include "util/list.hpp"

struct RenderContext;
struct VoxelMultiMeshEntry;
struct VoxelShape;
struct EntityPlant;

struct ModelFlower : public ModelEntity {
    using Base = ModelEntity;
    using Base::Base;

    bool render_flower(
        RenderContext &ctx,
        const EntityPlant &entity,
        const mat4 &model_base,
        usize i = 0) const;

    AABB aabb() const override;

    AABB entity_aabb(const Entity &entity) const override;

protected:
    virtual bool use_small_mesh(
        const EntityPlant &entity,
        usize i = 0) const = 0;

    virtual const VoxelMultiMeshEntry &get_mesh_small(
        const EntityPlant &entity,
        usize i = 0) const = 0;

    virtual const VoxelMultiMeshEntry &get_mesh_stem(
        const EntityPlant &entity,
        usize i = 0) const = 0;

    virtual const VoxelMultiMeshEntry &get_mesh_flower(
        const EntityPlant &entity,
        usize i = 0) const = 0;

    virtual void get_mesh_leaves(
        List<const VoxelMultiMeshEntry*> &dst,
        const EntityPlant &entity,
        usize i = 0) const = 0;

    mat4 model_sprite(
        const EntityPlant &entity,
        const mat4 &base,
        const VoxelMultiMeshEntry &mesh,
        usize i = 0) const;

    mat4 model_flower(
        const EntityPlant &entity,
        const mat4 &base,
        const VoxelMultiMeshEntry &mesh,
        const VoxelMultiMeshEntry &mesh_stem,
        usize i = 0) const;

    mat4 model_stem(
        const EntityPlant &entity,
        const mat4 &base,
        const VoxelMultiMeshEntry &mesh,
        usize i = 0) const;

    mat4 model_leaf(
        const EntityPlant &entity,
        const mat4 &base,
        const VoxelMultiMeshEntry &mesh,
        usize n_leaf,
        usize j = 0) const;

    // adds bumps/reshapes flower (literal "flowering part") to give texture
    void reshape_flower(VoxelShape &shape, usize i);
};
