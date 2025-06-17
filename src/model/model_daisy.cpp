#include "model/model_daisy.hpp"
#include "gfx/sprite_resource.hpp"
#include "gfx/color_resource.hpp"
#include "gfx/specular_textures.hpp"

static auto spritesheet =
    SpritesheetResource("entity_daisy", "entity/daisy.png", { 16, 16 });

ModelDaisy::ModelDaisy() : Base() {
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

    // leaves
    for (usize i = 0; i < 2; i++) {
        this->meshes_leaf.push_back(
            &this->mesh.add(
                VoxelModel::from_sprite(
                    sprites,
                    sprites[{ 1 + i, 2 }],
                    area_spec,
                    1,
                    VoxelModel::FLAG_MIN_BOUNDS)));
    }

    // stem
    this->meshes_stem.push_back(
        &this->mesh.add(
            VoxelModel::from_sprite(
                sprites,
                sprites[{ 0, 2 }],
                area_spec,
                1,
                VoxelModel::FLAG_MIN_BOUNDS)));
}

std::span<const vec3> ModelDaisy::colors() const {
    static auto colors =
        ColorResource([]() { return spritesheet[uvec2(0, 1)]; });
    return std::span(*colors);
}
