#include "model/model_flower.hpp"
#include "voxel/voxel_multi_mesh.hpp"
#include "entity/entity_plant.hpp"
#include "entity/entity_render.hpp"
#include "gfx/render_context.hpp"
#include "util/hash.hpp"
#include "util/rand.hpp"

static const auto
    AABB_MODEL = AABB({ 16, 16, 16 }),
    AABB_COLLISION =
        AABB({ 5, 10, 5 })
            .center_on(AABB_MODEL.center(), { true, false, true });

mat4 ModelFlower::model_sprite(
    const EntityPlant &entity,
    const mat4 &base,
    const VoxelMultiMeshEntry &mesh,
    usize i) const {
    auto m = base;
    auto rand = Rand(hash(entity.id, mesh.id));
    const auto
        size = mesh.size(),
        center = math::xyz_to_xnz(size, 0.0f) / 2.0f;
    m = math::translate(
        m,
        math::xyz_to_xnz(
            (AABB_MODEL.size() / 2.0f) - center, 0.0f));
    m = math::translate(m, center);
    m *= math::dir_to_rotation(
            math::xz_to_xyz(
                global.game->camera->rotation_direction(),
                0.0f));
    m = math::rotate(m, math::PI_4 * rand.next(-1.0f, 1.0f), vec3(0, 1, 0));
    m = math::translate(m, -center);
    return m;
}

mat4 ModelFlower::model_flower(
    const EntityPlant &entity,
    const mat4 &base,
    const VoxelMultiMeshEntry &mesh,
    const VoxelMultiMeshEntry &mesh_stem,
    usize i) const {
    const auto
        size_flower = mesh.size(),
        center_offset = vec3(size_flower.x, 0.0f, size_flower.y) / 2.0f;

    auto rand = Rand(hash(entity.id, mesh.id));
    auto m = base;
    m = math::translate(
        m, math::xyz_to_xnz(
            (math::xyz_to_xnz(AABB_MODEL.size(), 0.0f) / 2.0f) - center_offset,
            mesh_stem.size().y + 1.0f));
    m = math::translate(m, center_offset);
    m *= math::dir_to_rotation(
            math::xz_to_xyz(
                global.game->camera->rotation_direction(),
                0.0f));
    m = math::rotate(m, rand.next(math::PI_16, math::PI_16), vec3(0, 1, 0));
    m = math::rotate(m, -math::PI_8, vec3(1, 0, 0));
    m = math::translate(m, -center_offset);
    m = math::rotate(m, math::PI_2, vec3(1, 0, 0));
    return m;
}

mat4 ModelFlower::model_stem(
    const EntityPlant &entity,
    const mat4 &base,
    const VoxelMultiMeshEntry &mesh,
    usize i) const {
    const auto
        size_stem = mesh.size(),
        center_offset = vec3(size_stem.x, 0.0f, 0.0f) / 2.0f;

    auto rand = Rand(hash(entity.id, mesh.id));
    auto m = base;
    m = math::translate(
        m, (math::xyz_to_xnz(AABB_MODEL.size(), 0.0f) / 2.0f) - center_offset);
    m = math::translate(m, center_offset);
    m = math::rotate(m, rand.next(0.0f, math::TAU), vec3(0, 1, 0));
    m = math::translate(m, -center_offset);
    return m;
}

mat4 ModelFlower::model_leaf(
    const EntityPlant &entity,
    const mat4 &base,
    const VoxelMultiMeshEntry &mesh,
    usize n_leaf,
    usize i) const {
    const auto
        size_leaf = mesh.size(),
        center_offset = math::xyz_to_xnz(size_leaf, 0.0f) / 2.0f;

    auto rand = Rand(hash(entity.id, mesh.id, n_leaf, i, entity.pos));
    const auto disc_offset = (entity.id + i + n_leaf) * (math::TAU / 3.0f);
    const auto d =
        math::normalize(
            vec3(
                math::sin(disc_offset + rand.next(-0.2f, 0.2f)),
                rand.next(0.4f, 0.8f),
                math::cos(disc_offset + rand.next(-0.2f, 0.2f))));

    auto m = base;
    m = math::translate(m, -2.0f * math::xyz_to_xnz(d, 0.0f));
    m = math::translate(
        m,
        (math::xyz_to_xnz(AABB_MODEL.size(), 0.0f) / 2.0f)
            - math::xyz_to_xnz(center_offset, 0.0f));
    m = math::translate(m, vec3(0.0f, 1.0f, 0.0f));
    m = math::translate(m, center_offset);
    m *= math::dir_to_rotation(d);
    m = math::translate(m, -center_offset);
    return m;
}

// TODO: should not return bool
bool ModelFlower::render_flower(
    RenderContext &ctx,
    const EntityPlant &entity,
    const mat4 &model_base,
    usize i) const {
    const auto &plant = dynamic_cast<const EntityPlant&>(entity);

    const auto passes =
       entity_render::adjust_passes(
            RENDER_FLAG_PASS_ALL_3D & ~RENDER_FLAG_PASS_TRANSPARENT,
            entity);

    auto rand = Rand(hash(entity.id));

    // check if we can use small meshes
    if (this->use_small_mesh(plant)) {
        const auto &mesh = this->get_mesh_small(plant, i);
        ctx.push(
            ModelPart {
                .entry = &mesh,
                .model = this->model_sprite(plant, model_base, mesh, i),
                .passes = passes,
                .entity_id = entity.id
            });
        return true;
    }

    const auto
        &mesh_flower = this->get_mesh_flower(plant, i),
        &mesh_stem = this->get_mesh_stem(plant, i);

    ctx.push(
        ModelPart {
            .entry = &mesh_flower,
            .model =
                this->model_flower(
                    plant, model_base, mesh_flower, mesh_stem, i),
            .passes = passes,
            .entity_id = entity.id
        });
    ctx.push(
        ModelPart {
            .entry = &mesh_stem,
            .model =
                this->model_stem(plant, model_base, mesh_stem, i),
            .passes = passes,
            .entity_id = entity.id
        });

    List<const VoxelMultiMeshEntry*> meshes_leaf(&global.frame_allocator);
    this->get_mesh_leaves(meshes_leaf, plant, i);
    if (!meshes_leaf.empty()) {
        const auto n_leaves = rand.next<usize>(1, 3);

        for (usize i = 0; i < n_leaves; i++) {
            const auto &mesh_leaf = *rand.pick(meshes_leaf.span());
            ctx.push(
                ModelPart {
                    .entry = &mesh_leaf,
                    .model = this->model_leaf(plant, model_base, mesh_leaf, i),
                    .passes = passes,
                    .entity_id = entity.id
                });
        }
    }

    return true;
}

AABB ModelFlower::aabb() const {
    return AABB_MODEL;
}

AABB ModelFlower::entity_aabb(const Entity &entity) const {
    const auto &plant = dynamic_cast<const EntityPlant&>(entity);
    const auto &mesh_stem = this->get_mesh_stem(plant);
    return AABB::merge(
        AABB_COLLISION,
        mesh_stem.bounds()
            .center_on(AABB_MODEL.center(), bvec3(1, 0, 1)))
        .scale(1.0f / SCALE);
}

void ModelFlower::reshape_flower(VoxelShape &shape, usize i) {
    auto rand = Rand(hash(i));
    const auto copy = shape.deep_copy();

    const auto bounds = shape.bounds();
    const auto
        size_xy = bounds.size().xy(),
        center_xy = bounds.center().xy();

    shape.resize(uvec3(shape.size.xy(), 2));
    shape.each(
        [&](const ivec3 &pos) {
            shape[pos] = false;

            if (pos.z == 1 && copy[ivec3(pos.xy(), 0)]) {
                shape[pos] = true;
            } else if (pos.z == 0 && copy[pos]) {
                const auto d =
                    math::length((vec2(pos.xy()) - vec2(center_xy)))
                        / math::length(vec2(size_xy) / 2.0f);

                if (rand.next(0.0f, 0.4f) > d) {
                    shape[pos] = true;
                }
            }
        });
}
