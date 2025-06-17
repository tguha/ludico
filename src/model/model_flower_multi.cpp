#include "model/model_flower_multi.hpp"
#include "entity/entity_plant.hpp"
#include "entity/entity_render.hpp"
#include "util/hash.hpp"
#include "util/rand.hpp"

ModelFlowerMulti::ModelFlowerMulti()
    : mesh(&Renderer::get().allocator) {}

bool ModelFlowerMulti::use_small_mesh(
    const EntityPlant &plant,
   usize i) const {
    return plant.stage < this->meshes_small.size();
}

const VoxelMultiMeshEntry &ModelFlowerMulti::get_mesh_small(
    const EntityPlant &plant,
   usize i) const {
    ASSERT(this->meshes_small.size() > plant.stage);
    auto rand = Rand(hash(plant.id, plant.stage));
    return *rand.pick(this->meshes_small[plant.stage]);
}

const VoxelMultiMeshEntry &ModelFlowerMulti::get_mesh_stem(
    const EntityPlant &plant,
   usize i) const {
    ASSERT(!this->meshes_stem.empty());
    auto rand = Rand(hash(plant.id, plant.stage));
    return *rand.pick(this->meshes_stem);
}

const VoxelMultiMeshEntry &ModelFlowerMulti::get_mesh_flower(
    const EntityPlant &plant,
    usize i) const {
    const auto j = plant.stage - this->meshes_small.size();
    ASSERT(this->meshes_flower.size() > j);
    return *this->meshes_flower[j];
}

void ModelFlowerMulti::get_mesh_leaves(
    List<const VoxelMultiMeshEntry*> &dst,
    const EntityPlant &plant,
    usize i) const {
    dst.push(this->meshes_leaf);
}

bool ModelFlowerMulti::render(
    RenderContext &ctx,
    const EntityPlant &plant) const {

    const auto n = this->n_flowers(plant);
    for (usize i = 0; i < n; i++) {
        auto m_base = entity_render::model_base(plant);
        m_base = math::translate(mat4(1.0), this->offset(plant, i)) * m_base;
        if (!Base::render_flower(ctx, plant, m_base, i)) {
            return false;
        }
    }

    return true;
}
