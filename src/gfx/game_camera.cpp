#include "gfx/game_camera.hpp"
#include "entity/entity.hpp"
#include "gfx/texture_reader.hpp"
#include "util/ray.hpp"
#include "util/time.hpp"
#include "level/level.hpp"
#include "input/input.hpp"
#include "input/controls.hpp"
#include "platform/platform.hpp"
#include "platform/window.hpp"
#include "gfx/renderer.hpp"
#include "state/state_game.hpp"
#include "constants.hpp"
#include "global.hpp"

static GameCamera::Frustum compute_frustum(const GameCamera &camera) {
    /* static AABB compute_frustum(const GameCamera &camera) { */
    const auto size = ivec2(Renderer::get().size);

    // plane points from screen pixels for top/bottom/left/right
    const auto plane_points =
        types::make_array(
            types::make_array(
                (size - 1) * ivec2(0, 1),
                (size - 1) * ivec2(1, 1)),
            types::make_array(
                (size - 1) * ivec2(0, 0),
                (size - 1) * ivec2(1, 0)),
            types::make_array(
                (size - 1) * ivec2(0, 0),
                (size - 1) * ivec2(0, 1)),
            types::make_array(
                (size - 1) * ivec2(1, 0),
                (size - 1) * ivec2(1, 1)));

    GameCamera::Frustum frustum;

    bool fail = false;
    const auto aabb_l = camera.level->aabb();
    const auto compute_plane_center =
        [&](const auto &arr, std::span<vec3, std::dynamic_extent> hits) {
            for (usize i = 0; i < 2; i++) {
                const auto res =
                    camera.ray_from(arr[i])
                        .intersect_aabb(aabb_l, &hits[(i * 2) + 0]);
                if (!res) {
                    fail = true;
                }

                hits[(i * 2) + 1] = *res;
            }

            vec3 result = vec3(0.0);
            for (auto &h : hits) {
                result += h;
            }
            return result / vec3(4.0f);
        };

    if (fail) {
        return GameCamera::Frustum { };
    }

    // compute centers are store hits
    std::array<vec3, 16> hits;
    for (usize i = 0; i < 4; i++) {
        frustum.planes[i].c =
            compute_plane_center(
                plane_points[i],
                std::span(hits).subspan(i * 4, 4));
    }

    // rough AABB is merge of hits, slightly expanded
    frustum.box =
        AABB::compute(std::span(hits))
            .expand_center(vec3(2.0f))
            .clamp(aabb_l);

    // compute down/left vectors (screen down and screen right)
    const auto center = (size / 2);
    const auto
        n_down =
            math::normalize(
                camera.to_world_pos({ center.x, center.y - 1 })
                    - camera.to_world_pos(center)),
        n_left =
            math::normalize(
                camera.to_world_pos({ center.x - 1, center.y })
                    - camera.to_world_pos(center));

    frustum.plane_top.n = n_down;
    frustum.plane_bottom.n = -n_down;
    frustum.plane_right.n = n_left;
    frustum.plane_left.n = -n_left;

    // move each center along its inverse normal: adjust planes to allow some
    // wiggle room for larger objects
    for (auto &p : frustum.planes) {
        p.c += 1.5f * -p.n;
    }

    return frustum;
}

void GameCamera::follow(const Entity &entity) {
    const auto target =
        math::xz_to_xyz(
            entity.pos.xz()
                - (this->rotation_direction()
                        * math::max(entity.pos.y - 1.0f, 0.0f)),
            entity.pos.y);

    this->follow(target, 3.6f / f32(TICKS_PER_SECOND));
}

void GameCamera::follow(const vec3 &target, f32 speed) {
    const auto l = math::length(target - this->pos);
    if (l > 5.0f || l < 0.01f) {
        this->pos = target;
    } else {
        this->pos = math::lerp(this->pos, target, speed);
    }
}

void GameCamera::set_uniforms(const Program &program) const {
    program.try_set(
        "u_game_camera_pos", vec4(this->pos, 1.0f));

    constexpr auto
        U_SAMPLE_OFFSET = "u_game_camera_sample_offset",
        U_SAMPLE_RANGE = "u_game_camera_sample_range";

    // compute a UV sample offset for the final composite texture sampling
    // compute screen center in NDC space. origin_offset is the translation
    // offset for final texel space
    if (program.has(U_SAMPLE_OFFSET)) {
        const auto
            size = vec2(Renderer::get().size);

        const auto offset_for_camera = [&](const Camera &c, const vec3 &ref) {
            const auto
                p_ndc = c.project(ref).xy(),
                p_px = size * ((p_ndc + 1.0f) / 2.0f),
                p_px_r = math::round(p_px),
                p_offset = (p_px - p_px_r) * (1.0f / size);
            return p_offset;
        };

        program.try_set(
            U_SAMPLE_OFFSET,
            vec4(
                offset_for_camera(this->camera, this->pos).x,
                offset_for_camera(this->camera_unadjusted, vec3(0)).y,
                vec2(0)));
    }

    // compute ratio of (renderer_size) / (renderer_size + 1)
    if (program.has(U_SAMPLE_RANGE)) {
        const auto range =
            vec2(Renderer::get().target_size)
                / vec2(Renderer::get().size);
        program.try_set(
            "u_game_camera_sample_range",
            vec4(
                range,
                0.0f, 0.0f));
    }
}

void GameCamera::update() {
    // clamp position inside of level
    this->pos = math::clamp(
        this->pos, vec3(0.0f), this->level->aabb().max.xyz());

    const auto pos_r_xz = this->pos.xz();
    const auto zoom_scale = 1.0f + ZOOM_SCALE_PER_LEVEL * f32(this->zoom);

    // compute half-projection size scaled by projection scale, pixel scale,
    // and zoom scale
    const auto h_proj_size =
        (((vec2(Renderer::get().size) * PROJ_SCALE) / SCALE)
            * zoom_scale)
            / 2.0f;

    this->camera.proj =
        math::ortho(
            -h_proj_size.x, h_proj_size.x,
            -h_proj_size.y, h_proj_size.y,
            DEPTH_RANGE.x, DEPTH_RANGE.y);

    // apply camera offset as projection matrix translation
    // invert the offset so that positive offsets will move the camera
    // up and right relative to the center, not the opposite
    const auto offset_scaled = this->offset.value / SCALE;
    this->camera.proj =
        math::translate(
            this->camera.proj,
            vec3(-offset_scaled, 0.0f));

    // rotated center offset
    const auto rot = this->rotation_direction();

    // offset position by rotation
    const auto pos_3d =
        vec3(
            pos_r_xz.x - (rot.x * SCALE),
            SCALE,
            pos_r_xz.y - (rot.y * SCALE));

    // construct view matrix from rotation angles, translation
    auto view = mat4(1.0);
    view *= math::rotate(
        mat4(1.0), VIEW_ROT.x, vec3(1, 0, 0));
    view *= math::rotate(
        mat4(1.0), VIEW_ROT.y + this->rotation_angle(), vec3(0, 1, 0));
    view *= math::rotate(
        mat4(1.0), VIEW_ROT.z, vec3(0, 0, 1));
    view = math::translate(view, vec3(-pos_3d.x, -SCALE, -pos_3d.z));
    this->camera.view = view;

    // save camera pre-texel rounding
    this->camera_unadjusted = camera;

    // use shadow-shimmer rounding technique to ensure pixels (and texture
    // samples!) end up in the same place
    // stackoverflow.com/questions/33499053/cascaded-shadow-map-shimmering
    const auto size_4 = vec4(Renderer::get().size, vec2(1));
    auto origin_s =
        (this->camera.proj * this->camera.view) *
            vec4(vec3(0.0), 1.0);
    origin_s *= size_4 / 2.0f;

    auto origin_r = math::round(origin_s);
    auto offset = origin_r - origin_s;

    offset *= 2.0f / size_4;
    offset.z = 0;
    offset.w = 0;
    this->camera.proj[3] += offset;

    this->camera.update();
    this->camera_unadjusted.update();
    this->frustum = compute_frustum(*this);
}

std::optional<std::tuple<ivec3, Direction::Enum, vec3>> GameCamera::tile_at(
    const ivec2 &pos) const {
    const auto
        &proj = this->camera.proj,
        &view = this->camera.view;

    // clip space position
    const auto pos_clip =
        ((vec2(pos) * 2.0f) / vec2(Renderer::get().size)) - 1.0f;

    // world space pos
    const auto pos_w =
        (math::inverse(view) * math::inverse(proj) * vec4(pos_clip, 0, 1))
            .xyz();

    const auto ray = math::Ray(pos_w, this->direction());

    vec3 hit;
    const auto res =
        ray.intersect_block(
            [&](const ivec3 &pos) {
                return
                    Tiles::get()[this->level->tiles[pos]]
                        .solid(*this->level, pos);
            },
            GameCamera::DEPTH_RANGE.y,
            &hit);

    return res ?
        std::make_optional(
            std::make_tuple(
                std::get<0>(*res),
                std::get<1>(*res),
                hit))
        : std::nullopt;
}

EntityRef GameCamera::entity_at(
    const ivec2 &pos,
    usize padding) const {
    std::unordered_map<EntityId, usize> hits;

    for (int y = -padding; y <= static_cast<int>(padding); y++) {
        for (int x = -padding; x <= static_cast<int>(padding); x++) {
            const auto p = pos + ivec2(x, y);
            if (p.x < 0
                || p.y < 0
                || p.x >= isize(Renderer::get().size.x)
                || p.y >= isize(Renderer::get().size.y)) {
                continue;
            }

            const auto id =
                static_cast<EntityId>(
                    Renderer::get().data_reader->get<vec4>(uvec2(p)).b);

            if (id == NO_ENTITY) {
                continue;
            }

            hits[id]++;
        }
    }

    if (hits.size() == 0) {
        return EntityRef::none();
    }

    // find "most hit" entity ID
    const auto id =
        std::max_element(
            hits.begin(), hits.end(),
            [](const auto &a, const auto &b) {
                return a.second < b.second;
            })->first;

    return EntityRef(this->level->entity_from_id(id));
}

void GameCamera::tick(const Level &level) {
    const auto &mouse = global.platform->get_input<Mouse>();
    this->_mouse_tile = this->tile_at(mouse.pos);
    this->_mouse_entity = this->entity_at(mouse.pos, 2);

    // update offset
    const auto l_offset =
        math::length(this->offset.target - this->offset.value);
    if (l_offset > 128.0f || l_offset < 0.01f) {
        this->offset.value = this->offset.target;
    } else {
        this->offset.value =
            math::lerp(
                this->offset.value,
                this->offset.target,
                this->offset.speed);
    }
}

std::optional<std::tuple<ivec3, Direction::Enum, vec3>>
    GameCamera::mouse_tile() const {
    return this->_mouse_tile;
}

EntityRef GameCamera::mouse_entity() const {
    return this->_mouse_entity;
}

bool GameCamera::is_entity_occluded(const Entity &e) const {
    // TODO: better check than just center?
    const auto screen_pos =
        this->to_screen_pos(e.center);

    if (!screen_pos) {
        return true;
    }

    const auto f = this->entity_at(*screen_pos, 1);
    return !f || (*f != e);
}

AABB2i GameCamera::light_bounds() const {
    const auto frustum_tile = Level::aabb_to_tile(this->frustum.aabb());
    return AABB2i(
        frustum_tile.min.xz() - ivec2(LIGHT_MAX_VALUE),
        frustum_tile.max.xz() + ivec2(LIGHT_MAX_VALUE))
        .clamp(this->level->aabb_tile());
}

AABB2i GameCamera::render_bounds() const {
    const auto frustum_tile = Level::aabb_to_tile(this->frustum.aabb());
    return AABB2i(
        frustum_tile.min.xz() - ivec2(4),
        frustum_tile.max.xz() + ivec2(4))
        .clamp(this->level->aabb_tile());
}

// returns true if entity e is potentially on screen, or close
bool GameCamera::possibly_visible(const Entity &e) const {
    return this->frustum.contains(e.center);
}

std::optional<ivec2> GameCamera::to_screen_pos(const vec3 &pos_w) const {
    // convert position to clip space
    const auto pos_clip = this->camera.view_proj * vec4(pos_w, 1.0f);

    if (math::abs(pos_clip.x) > pos_clip.w
        || math::abs(pos_clip.y) > pos_clip.w
        || pos_clip.z < 0
        || pos_clip.z > pos_clip.w) {
        return std::nullopt;
    }

    // convert from 3D [-1, 1] to 2D [0, 1]
    const auto pos_n = pos_clip.xy() * 0.5f + 0.5f;

    // multiply by screen size for final position
    return ivec2(math::floor(vec2(Renderer::get().size) * pos_n));
}

vec3 GameCamera::to_world_pos(const ivec2 &pos_s) const {
    if (!this->in_screen_bounds(pos_s)) {
        return vec3(0);
    }

    const auto pos_ndc =
        vec4(
            ((vec2(pos_s) / vec2(Renderer::get().size)) * 2.0f) - 1.0f,
            0.0f,
            1.0f);
    return (this->camera.inv_view_proj * pos_ndc).xyz();
}

Camera GameCamera::compute_rounded_camera(const vec3 &pos) const {
    const auto size_4 = vec4(Renderer::get().size, vec2(1));
    auto origin_s = (this->camera_unadjusted.view_proj) * vec4(pos, 1.0);
    origin_s *= size_4 / 2.0f;

    auto origin_r = math::round(origin_s);
    auto offset = origin_r - origin_s;

    offset *= 2.0f / size_4;
    offset.z = 0;
    offset.w = 0;

    // compute initial adjustment relative to unadjusted camera, this position
    // needs a custom projection adjustment
    Camera c = this->camera_unadjusted;
    c.proj[3] += offset;
    c.update();

    // find camera offset of position, use to adjust view matrix again
    // NOTE: use the ADJUSTED CAMERA here, we need to find the offset relative
    // to everything else in the world
    const auto
        pos_p0 = (this->camera.view_proj * vec4(pos, 1.0f)).xyz(),
        pos_p1 = (c.view_proj * vec4(pos, 1.0f)).xyz();

    // compute offset in pixels from original position
    const auto
        pos_pp0 = math::round(pos_p0.xy() * (size_4 / 2.0f).xy()),
        pos_pp1 = math::round(pos_p1.xy() * (size_4 / 2.0f).xy()),
        diff_px = pos_pp1 - pos_pp0;

    // offset again, this time in whole pixels to return pos, now rounded, to
    // its original pixel location, undoing any whole-pixel offsets which were
    // done by the first adjustment while maintaining sub-pixel offsets
    Camera d = c;
    d.proj[3] -= vec4(diff_px * (2.0f / size_4).xy(), vec2(0.0f));
    d.update();

    return d;
}

std::optional<AABB2u> GameCamera::to_screen_bounds(const AABB &aabb) const {
    usize n = 0;
    std::array<uvec2, 8> target_points;

    for (const auto &p_ws : aabb.points()) {
        if (const auto p_px = this->to_screen_pos(p_ws)) {
            target_points[n++] = *p_px;
        }
    }

    if (n == 0) {
        return std::nullopt;
    }

    // convert to screen space
    return AABB2u::compute((std::span { target_points }).subspan(0, n));
}

bool GameCamera::in_screen_bounds(const ivec2 &pos_s) const {
    return AABB2i(ivec2(0), ivec2(Renderer::get().size - uvec2(1)))
        .contains(pos_s);
}
