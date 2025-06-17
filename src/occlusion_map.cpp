#include "occlusion_map.hpp"
#include "entity/entity.hpp"
#include "gfx/debug_renderer.hpp"
#include "gfx/game_camera.hpp"
#include "gfx/program.hpp"
#include "gfx/renderer.hpp"
#include "gfx/texture_reader.hpp"
#include "state/state_game.hpp"
#include "level/level.hpp"
#include "util/rand.hpp"
#include "util/ray.hpp"
#include "global.hpp"

// include from shaders
#include "occlusion_map.sc"

void OcclusionMap::init() {
    // one byte per tile per layer
    this->data_full = std::vector<u8>(math::prod(SIZE));
    this->data_top = std::vector<u8>(math::prod(SIZE.xz()));
    this->data_blocking = std::vector<u8>(math::prod(SIZE));
    this->texture_full =
        Texture(
            bgfx::createTexture3D(
                SIZE.x, SIZE.y, SIZE.z,
                false,
                bgfx::TextureFormat::R8,
                BGFX_SAMPLER_MIN_POINT
                | BGFX_SAMPLER_MAG_POINT
                | BGFX_SAMPLER_U_CLAMP
                | BGFX_SAMPLER_V_CLAMP),
            uvec2(0));
    this->texture_top =
        Texture(
            bgfx::createTexture2D(
                SIZE.x, SIZE.z,
                false, 1,
                bgfx::TextureFormat::R8,
                BGFX_SAMPLER_MIN_POINT
                | BGFX_SAMPLER_MAG_POINT
                | BGFX_SAMPLER_U_CLAMP
                | BGFX_SAMPLER_V_CLAMP),
            uvec2(0));
    this->texture_blocking =
        Texture(
            bgfx::createTexture3D(
                SIZE.x, SIZE.y, SIZE.z,
                false,
                bgfx::TextureFormat::R8,
                BGFX_SAMPLER_MIN_POINT
                | BGFX_SAMPLER_MAG_POINT
                | BGFX_SAMPLER_U_CLAMP
                | BGFX_SAMPLER_V_CLAMP),
            uvec2(0));
}

static void compute_blocking(
    OcclusionMap &occlusion_map,
    const GameCamera &camera,
    const Level &level) {

    // scale applied to target AABB to stop rays
    constexpr auto TARGET_AABB_SCALE = 1.2f;

    // returns true if position has tile and tile is solid
    const auto is_solid =
        [&](const ivec3 &pos) {
            return Tiles::get()[level.tiles[pos]].solid(level, pos);
        };

    std::fill(
        occlusion_map.data_blocking.begin(),
        occlusion_map.data_blocking.end(),
        0);

    const auto &target = *OPT_OR_RET(global.game->camera_entity.opt());
    const auto aabb_target = OPT_OR_RET(target.aabb());

    // maximum ray travel distance
    const auto ray_distance =
        math::length(
            camera.center_world_space()
                - target.center) + 4.0f;

    // screen space AABB
    const auto aabb_px =
        OPT_OR_RET(
            camera.to_screen_bounds(
                aabb_target.scale_center(TARGET_AABB_SCALE)));

    std::vector<ivec3> blocking;

    // try to raycast from every screen pixel occupied by the entity stencil,
    // using aabb_px as a filter area
    for (usize y = aabb_px.min.y; y <= aabb_px.max.y; y++) {
        for (usize x = aabb_px.min.x; x <= aabb_px.max.x; x++) {
            // filter out pixels which aren't exactly the target entity
            if (x >= Renderer::get().size.x
                || y >= Renderer::get().size.y
                || EntityId(
                    Renderer::get().entity_stencil_reader
                        ->get<f32>(uvec2(x, y))) != target.id) {
                continue;
            }

            // cast ray
            const auto ray =
                math::Ray(
                    camera.to_world_pos(ivec2(x, y)),
                    camera.direction());

            ray.intersect_block(
                [&](const ivec3 &pos) {
                    if (!level.in_bounds(pos)) {
                        return false;
                    }

                    // return true (stop ray) if entity is hit
                    const auto hit_result =
                        AABB::unit()
                            .translate(vec3(pos))
                            .collides(
                                aabb_target.scale_center(TARGET_AABB_SCALE));

                    const auto pos_d = pos - occlusion_map.offset;
                    if (!AABBi(ivec3(OcclusionMap::SIZE) - 1)
                            .contains(pos_d)
                            || !is_solid(pos)) {
                        return hit_result;
                    }

                    // check true collision with tile AABB
                    if (const auto aabb =
                            Tiles::get()[level.tiles[pos]]
                                .aabb(level, pos)) {
                        if (!ray.intersect_aabb(*aabb)) {
                            return hit_result;
                        }
                    }

                    // tile is blocking
                    occlusion_map.data_blocking[
                        (pos_d.z
                            * OcclusionMap::SIZE.y
                            * OcclusionMap::SIZE.x)
                            + (pos_d.y * OcclusionMap::SIZE.x)
                            + pos_d.x] = 0xFF;

                    blocking.push_back(pos);

                    return hit_result;
                }, ray_distance);
        }
    }

    // expand blocking tiles to include neighbors
    auto ds = camera.rotation_directions();
    std::transform(ds.cbegin(), ds.cend(), ds.begin(), Direction::opposite);

    for (usize i = 0, n = blocking.size(); i < n; i++) {
        const auto pos = blocking[i];
        for (const auto &d : ds) {
            const auto pos_n = pos + Direction::to_ivec3(d);
            if (!level.in_bounds(pos_n)) {
                continue;
            }

            blocking.push_back(pos_n);
        }
    }

    // expand blocking tiles to include everything above them
    for (usize i = 0, n = blocking.size(); i < n; i++) {
        const auto pos = blocking[i];
        for (usize y = pos.y; y < Chunk::SIZE.y; y++) {
            const auto pos_n = ivec3(pos.x, y, pos.z);
            blocking.push_back(pos_n);
        }
    }

    // update blocking map
    for (const auto &pos : blocking) {
        auto it = occlusion_map.blocking.find(pos);
        if (it != occlusion_map.blocking.end()) {
            auto &[_, info] = *it;
            info.last = global.time->ticks;
        } else {
            occlusion_map.blocking[pos] = {
                global.time->ticks,
                global.time->ticks
            };
        }
    }
}

static void apply_blocking(
    OcclusionMap &occlusion_map,
    Level &level) {
    // - remove all tiles under threshold
    // - tiles which have existed in map for over 5 ticks are ghosted
    for (auto it = occlusion_map.blocking.begin();
         it != occlusion_map.blocking.end();) {
        const auto &[pos, info] = *it;
        if ((info.last + 5) < global.time->ticks) {
            level.ghost[pos] = false;
            it = occlusion_map.blocking.erase(it);
        } else {
            if ((info.first + 20) < global.time->ticks && !level.ghost[pos]) {
                level.ghost[pos] = true;
            }

            it++;
        }
    }
}

void OcclusionMap::update(
    const GameCamera &camera,
    Level &level,
    const ivec2 &center) {
    this->offset = math::xz_to_xyz(center - ivec2(SIZE.xz() / 2u), 0);

    for (usize z = 0; z < SIZE.z; z++) {
        for (usize y = 0; y < SIZE.y; y++) {
            for (usize x = 0; x < SIZE.x; x++) {
                const auto
                    index_3d = (z * SIZE.y * SIZE.x) + (y * SIZE.x) + x,
                    index_2d = (z * SIZE.x) + x;

                const auto p = this->offset + ivec3(x, y, z);
                const auto &tile = Tiles::get()[level.tiles[p]];

                const auto occupied =
                    !(tile == 0
                        || tile.transparency_type() != Tile::Transparency::OFF);
                this->data_full[index_3d] = occupied ? 0xFF : 0x00;

                // compute top occlusion
                if (y == SIZE.y - 1) {
                    // check neighbors: if none, then mark occluded
                    bool is_hidden = true;
                    for (const auto &d : Direction::ALL) {
                        const auto n = p + Direction::to_ivec3(d);

                        if (!Level::in_bounds(n)) {
                            continue;
                        }

                        if (Tiles::get()[level.tiles[n]].transparency_type()
                                != Tile::Transparency::OFF) {
                            is_hidden = false;
                            break;
                        }
                    }

                    this->data_top[index_2d] = is_hidden ? 0xFF : 0x00;
                }
            }
        }
    }

    compute_blocking(
        *this,
        camera,
        level);

    apply_blocking(
        *this,
        level);

    bgfx::updateTexture3D(
        this->texture_full,
        0, 0, 0, 0,
        SIZE.x, SIZE.y, SIZE.z,
        bgfx::makeRef(
            &this->data_full[0],
            this->data_full.size() * sizeof(this->data_full[0])));

    bgfx::updateTexture2D(
        this->texture_top,
        0, 0, 0, 0,
        SIZE.x, SIZE.z,
        bgfx::makeRef(
            &this->data_top[0],
            this->data_top.size() * sizeof(this->data_top[0])));

    bgfx::updateTexture3D(
        this->texture_blocking,
        0, 0, 0, 0,
        SIZE.x, SIZE.y, SIZE.z,
        bgfx::makeRef(
            &this->data_blocking[0],
            this->data_blocking.size() * sizeof(this->data_blocking[0])));
}

void OcclusionMap::set_uniforms(
    const Program &program,
    u8 sampler_stage_full,
    u8 sampler_stage_top,
    u8 sampler_stage_blocking) const {
    program.try_set(
        STRINGIFY(OCCM_FULL_SAMPLER_NAME),
        sampler_stage_full,
        this->texture_full);
    program.try_set(
        STRINGIFY(OCCM_TOP_SAMPLER_NAME),
        sampler_stage_top,
        this->texture_top);
    program.try_set(
        STRINGIFY(OCCM_BLOCKING_SAMPLER_NAME),
        sampler_stage_blocking,
        this->texture_blocking);
    program.try_set(
        STRINGIFY(OCCM_OFFSET_UNIFORM_NAME),
        vec4(this->offset, 0.0f));
    program.try_set(
        STRINGIFY(OCCM_SIZE_UNIFORM_NAME),
        vec4(OcclusionMap::SIZE, 0.0f));
}
