#include "tile/tile_renderer_ore.hpp"
#include "util/cone.hpp"
#include "util/hash.hpp"
#include "util/rand.hpp"
#include "constants.hpp"

static void reshape(u64 seed, VoxelShape &shape) {
    auto rand = Rand(hash(seed));

    // generate random cones, all eminiating from (about) the middle
    // (in world space, not pixel space)
    std::array<math::Cone, 4> cones;
    for (usize i = 0; i < cones.size(); i++) {
        const auto bottom =
            vec3(
                0.5f + rand.next(-0.3f, 0.3f),
                0.0f,
                0.5f + rand.next(-0.3f, 0.3f));

        // pick a point on the top (ish) top point to
        // dir_r is radius of circle, from center, on the top of the cube
        // dir_x is the range of this quadrant (of the 4 quadrants of a
        // circle) where the direction point is picked
        const auto dir_r = rand.next(0.9f, 1.0f);
        const auto dir_x = rand.next(0.3f, 0.7f);
        const auto dir_p = (i + dir_x) * math::PI_2;
        const auto dir =
            math::normalize(
                vec3(
                    dir_r * ((math::cos(dir_p) + 1.0f) / 2.0f),
                    rand.next(0.9f, 1.0f),
                    dir_r * ((math::sin(dir_p) + 1.0f) / 2.0f))
                - bottom);

        const auto height = rand.next(0.4f, 0.7f);
        const auto radius = rand.next(0.1f, 0.4f);
        cones[i] =
            math::Cone(
                bottom + (height * dir),
                bottom,
                radius);
    }

    // check all points in shape for presence in cones
    VoxelShape copy = shape.deep_copy();
    copy.each(
        [&](const ivec3 &pos) {
            const auto c = (vec3(pos) + vec3(0.5f)) / SCALE;

            shape[pos] = false;
            for (usize i = 0; i < cones.size(); i++) {
                if (cones[i].contains(c)) {
                    shape[pos] = true;
                    break;
                }
            }
        });
}

TileRendererOre::TileRendererOre(
    const Tile &tile,
    const TextureArea &area_tex,
    const TextureArea &area_spec,
    u64 seed)
    : Base(tile) {
    for (usize i = 0; i < models.size(); i++) {
        this->models[i] =
            VoxelModel::from_sprites(
                *area_tex.parent,
                Direction::expand_directional_array<
                    std::array<std::optional<TextureArea>, Direction::COUNT>>(
                        std::make_optional(area_tex)),
                Direction::expand_directional_array<
                    std::array<std::optional<TextureArea>, Direction::COUNT>>(
                        std::make_optional(area_spec)),
                {},
                VoxelModel::FLAG_NONE,
                [&](VoxelShape &shape) {
                    reshape(hash(seed, i), shape);
                });
    }
}
