#include "model/model_hydrangea.hpp"
#include "gfx/sprite_resource.hpp"
#include "gfx/color_resource.hpp"
#include "gfx/specular_textures.hpp"
#include "entity/entity_plant.hpp"
#include "entity/entity_hydrangea.hpp"
#include "util/rand.hpp"
#include "util/hash.hpp"

static auto spritesheet =
    SpritesheetResource("entity_hydrangea", "entity/hydrangea.png", { 16, 16 });

ModelHydrangea::ModelHydrangea() : Base() {
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
    for (usize i = 0; i < 4; i++) {
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

const VoxelMultiMeshEntry &ModelHydrangea::get_mesh_flower(
    const EntityPlant &plant,
    usize i) const {
    ASSERT(plant.stage >= EntityHydrangea::STAGE_2);
    auto rand = Rand(hash(plant.id, plant.stage));
    return *this->meshes_flower[
        (plant.stage < EntityHydrangea::STAGE_5 ?
            0 : 2)
            + rand.next(0, 1)];
}

usize ModelHydrangea::n_flowers(const EntityPlant &plant) const {
    // small flowers + first "big" stage
    if (plant.stage < 3) {
        return 1;
    }

    return (plant.stage - 1) + Rand(plant.id).next(-1, 1);
}

vec3 ModelHydrangea::offset(const EntityPlant &plant, usize i) const {
    const auto n = this->n_flowers(plant);

    if (n == 1) {
        return vec3(0);
    }

    auto rand = Rand(hash(plant.id, i));

    if (n == 2) {
        // positions on disc radius
        constexpr auto RADIUS = 0.27f;
        const auto x = rand.next(0.0f, math::TAU);
        return RADIUS * vec3(math::cos(x), 0.0f, math::sin(x));
    }

    // return random positions in radius
    const auto radius = 0.3f + (0.1f * i);
    return vec3(rand.next(-radius, radius), 0.0f, rand.next(-radius, radius));
}

std::span<const vec3> ModelHydrangea::colors() const {
    static auto colors =
        ColorResource([]() { return spritesheet[uvec2(0, 1)]; });
    return std::span(*colors);
}
