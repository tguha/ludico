#include "model/model_sunflower.hpp"
#include "gfx/sprite_resource.hpp"
#include "gfx/color_resource.hpp"
#include "gfx/specular_textures.hpp"
#include "gfx/renderer_resource.hpp"
#include "gfx/multi_mesh_instancer.hpp"
#include "entity/entity_plant.hpp"

static auto spritesheet =
    SpritesheetResource("entity_sunflower", "entity/sunflower.png", { 16, 16 });

ModelSunflower::ModelSunflower() : Base() {
    const auto &sprites = spritesheet.get();
    const auto &area_spec = SpecularTextures::instance().min();

    // small and medium meshes
    this->meshes_small.resize(2);

    // small meshes
    for (usize i = 0; i < 2; i++) {
        this->meshes_small[0].push_back(
            &this->mesh.add(
                VoxelModel::from_sprite(
                    sprites,
                    sprites[{ i, 0 }],
                    area_spec,
                    1,
                    VoxelModel::FLAG_MIN_BOUNDS)));
    }

    // mid meshes
    for (usize i = 0; i < 2; i++) {
        this->meshes_small[1].push_back(
            &this->mesh.add(
                VoxelModel::from_sprite(
                    sprites,
                    sprites[{ 2 + i, 0 }],
                    area_spec,
                    1,
                    VoxelModel::FLAG_MIN_BOUNDS)));
    }

    // leaves
    for (usize i = 0; i < 2; i++) {
        this->meshes_leaf.push_back(
            &this->mesh.add(
                VoxelModel::from_sprite(
                    sprites,
                    sprites[{ 2 + i, 2 }],
                    area_spec,
                    1,
                    VoxelModel::FLAG_MIN_BOUNDS)));
    }

    // stem
    for (usize i = 0; i < 2; i++) {
        this->meshes_stem.push_back(
            &this->mesh.add(
                VoxelModel::from_sprite(
                    sprites,
                    sprites[{ i, 2 }],
                    area_spec,
                    1,
                    VoxelModel::FLAG_MIN_BOUNDS)));
    }

    // flowers
    for (usize i = 0; i < 3; i++) {
        this->meshes_flower.push_back(
            &this->mesh.add(
                VoxelModel::from_sprite(
                    sprites,
                    sprites[{ i, 1 }],
                    area_spec,
                    1,
                    VoxelModel::FLAG_MIN_BOUNDS,
                    [&](VoxelShape &shape) { reshape_flower(shape, i); })));
    }


}

std::span<const vec3> ModelSunflower::colors() const {
    static auto colors =
        ColorResource([]() { return spritesheet[uvec2(0, 1)]; });
    return std::span(*colors);
}

const VoxelMultiMeshEntry &ModelSunflower::get_mesh_stem(
    const EntityPlant &plant,
    usize i) const {
    return *this->meshes_stem[plant.stage <= 2 ? 0 : 1];
}
