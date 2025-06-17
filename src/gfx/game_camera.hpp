#pragma once

#include "util/ray.hpp"
#include "util/types.hpp"
#include "util/util.hpp"
#include "util/math.hpp"
#include "util/camera.hpp"
#include "util/direction.hpp"
#include "entity/entity_ref.hpp"
#include "gfx/zoom.hpp"

struct Level;

struct GameCamera {
    struct Frustum {
        struct Plane {
            vec3 c, n;
        };

        union {
            std::array<Plane, 4> planes;

            struct {
                Plane plane_top, plane_bottom, plane_left, plane_right;
            };
        };

        AABB box;

        inline bool contains(const vec3 &v) const {
            // check that v is contained within frustum planes
            for (const auto &p : this->planes) {
                if (math::dot(v - p.c, p.n) < 0.0f) {
                    return false;
                }
            }

            // basic AABB test for other axes
            return box.contains(v);
        }

        inline AABB aabb() const {
            return this->box;
        }
    };

    enum Rotation {
        ROT_SE = 0,
        ROT_SW,
        ROT_NW,
        ROT_NE,
        ROT_COUNT = (ROT_NE + 1)
    };

    static constexpr std::array<vec2, ROT_COUNT> ROTATIONS = {
        vec2( 1,  1),
        vec2(-1,  1),
        vec2(-1, -1),
        vec2( 1, -1)
    };

    static constexpr std::array<f32, ROT_COUNT> ROTATION_ANGLES = {
        0.0f,
        math::PI_2,
        math::PI,
        math::PI + math::PI_2
    };

    static inline const auto
        DEPTH_RANGE = vec2(0.01f, 64.0f);

    // view rotation on x/y/z axes
    // creates a dimetric projection rotated around the x and y axes
    static const inline auto VIEW_ROT =
        vec3(
            -math::atan(math::sin(math::radians(30.0f))),
            -math::radians(45.0f),
            0.0f);

    static inline const auto PROJ_SCALE =
        vec2(1.0f, math::sqrt(3.0f / 4.0f));

    // increase/decrease in PROJ_SCALE for each zoom level
    static constexpr auto ZOOM_SCALE_PER_LEVEL = 0.25f;

    Level *level;

    // current camera look-at position
    vec3 pos = vec3(0.0f);

    static constexpr auto OFFSET_DEFAULT_SPEED = 0.025f;

    // camera offset in pixels from pos
    // TODO: replace with a stack of values?
    struct {
        vec2 value = vec2(0.0f), target = vec2(0.0f);
        f32 speed = 0.025f;
    } offset;

    // primary camera
    Camera camera;

    // primary camera, pre texel rounding adjustment
    Camera camera_unadjusted;

    // view frustum, computed with camera update
    Frustum frustum;

    Zoom zoom = Zoom::ZOOM_DEFAULT;
    Rotation rotation = ROT_SE;

    GameCamera() = default;
    explicit GameCamera(Level &level)
        : level(&level) {}

    void set_uniforms(const Program &program) const;

    void follow(const Entity &entity);

    void follow(const vec3 &target, f32 speed);

    void update();

    void tick(const Level &level);

    // gets tile at specified screen position
    std::optional<std::tuple<ivec3, Direction::Enum, vec3>> tile_at(
        const ivec2 &pos) const;

    // current mouse tile/face/hit pos, if present
    std::optional<std::tuple<ivec3, Direction::Enum, vec3>> mouse_tile() const;

    // gets entity at specified screen position, EntityRef::none() if no such
    // entity
    EntityRef entity_at(const ivec2 &pos, usize padding = 2) const;

    // current mouse entity, if present. nullptr if not.
    EntityRef mouse_entity() const;

    // returns true if the specified entity cannot be seen by the camera
    // NOTE: also returns true if entity is off screen!
    bool is_entity_occluded(const Entity &e) const;

    // reasonable world-space bounds for culling lights
    AABB2i light_bounds() const;

    // reasonable world-space bounds for culling entities
    AABB2i render_bounds() const;

    // returns true if entity e is potentially on screen, or close
    // NOTE: does not check occlusion! (see is_entity_occluded)
    bool possibly_visible(const Entity &e) const;

    // convert world-space position to screen space position (2D)
    std::optional<ivec2> to_screen_pos(const vec3 &pos_w) const;

    // convert screen-space position to world space position (3D)
    vec3 to_world_pos(const ivec2 &pos_s) const;

    // compute rounded camera (view/proj) for object at position such that it
    // snaps to camera texels, based on the current view/proj for the camera
    Camera compute_rounded_camera(const vec3 &pos) const;

    // convert an aabb to 2D on-screen pixel bounds
    std::optional<AABB2u> to_screen_bounds(const AABB &aabb) const;

    // ray from screen pixel
    math::Ray ray_from(const ivec2 &pos_s) const {
        return math::Ray { this->to_world_pos(pos_s), this->direction() };
    }

    // world-space screen center position
    inline vec3 center_world_space() const {
        return
            (math::inverse(this->camera_unadjusted.view_proj)
                * vec4(0.0f, 0.0f, 0.0f, 1.0f)).xyz();
    }

    // returns true if position is valid screen pixel
    bool in_screen_bounds(const ivec2 &pos_s) const;

    // current direction vector
    inline vec3 direction() const {
        // extract from 3rd row of view matrix
        const auto v = this->camera.view;
        return math::normalize(vec3(v[0][2], v[1][2], v[2][2]));
    }

    // current rotation as sign vec2
    inline vec2 rotation_direction() const {
        return ROTATIONS[this->rotation];
    }

    // current rotation as angle
    inline f32 rotation_angle() const {
        return ROTATION_ANGLES[this->rotation];
    }

    inline std::array<Direction::Enum, 2> rotation_directions() const {
        const auto r = ivec2(this->rotation_direction());
        return types::make_array(
            Direction::from_ivec3({ -r.x, 0, 0 }),
            Direction::from_ivec3({ 0, 0, -r.y }));
    }

private:
    EntityRef _mouse_entity;
    std::optional<std::tuple<ivec3, Direction::Enum, vec3>> _mouse_tile;
};
