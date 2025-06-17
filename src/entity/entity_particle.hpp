#pragma once

#include "entity/entity.hpp"
#include "util/time.hpp"
#include "util/hash.hpp"
#include "util/rand.hpp"
#include "tile/tile_renderer.hpp"
#include "state/state_game.hpp"
#include "level/level.hpp"
#include "constants.hpp"
#include "global.hpp"

struct EntityParticle : public Entity {
    using Base = Entity;

    // particle color
    vec4 color;

    // ticks until particle disappears
    isize ticks_to_live = 120;

    EntityParticle() = default;

    EntityParticle(
        const vec4 &color)
        : Base(),
          color(color) {}

    EntityParticle(
        const vec3 &color)
        : EntityParticle(vec4(color, 1.0f)) {}

    template <typename T, typename V>
        requires std::is_base_of_v<EntityParticle, T>
    static IEntityRef<T> spawn(
        Level &level,
        const vec3 &pos,
        const V &color) {
        return level.spawn<T>(pos, color);
    }

    template <typename T, typename V>
        requires std::is_base_of_v<EntityParticle, T>
            && (std::is_same_v<V, vec3> || std::is_same_v<V, vec4>)
    static IEntityRef<T> spawn(
        Level &level,
        const vec3 &pos,
        usize min,
        usize max,
        const V &color) {
        std::array<V, 1> colors = { color };
        std::array<IEntityRef<T>, 1> refs = IEntityRef<T>::none();
        EntityParticle::spawn<T>(
            level, pos, min, max,
            std::span<const V> { colors },
            std::span<IEntityRef<T>> { refs });
        return refs[0];
    }

    template <typename T, typename V>
        requires std::is_base_of_v<EntityParticle, T>
            && (std::is_same_v<V, vec3> || std::is_same_v<V, vec4>)
    static void spawn(
        Level &level,
        const vec3 &pos,
        usize min,
        usize max,
        std::span<const V> colors,
        std::optional<std::span<IEntityRef<T>>> refs_out = std::nullopt) {
        constexpr auto V_IS_VEC3 = std::is_same_v<V, vec3>;
        if (colors.empty()) {
            return;
        }

        usize i = 0;
        auto rand = Rand(hash(pos, min, max, global.time->ticks));
        rand.n_times(
            min, max,
            [&]() {
                const auto selected =
                    colors[rand.next<int>(0, colors.size() - 1)];

                // convert vec3|vec4 -> vec4
                vec4 c = selected.rgb;
                if constexpr (V_IS_VEC3) {
                    c.a = 1.0f;
                } else {
                    c.a = selected.a;
                }

                const auto ref = level.spawn<T>(pos, c);
                if (refs_out && i < refs_out->size()) {
                    (*refs_out)[i++] = ref;
                }
            });
    }

    template <typename T>
        requires std::is_base_of_v<EntityParticle, T>
    static void spawn(
        Level &level,
        const vec3 &pos,
        usize min,
        usize max,
        std::span<const vec3> colors,
        std::optional<std::span<IEntityRef<T>>> refs_out = std::nullopt) {
        if (colors.empty()) {
            return;
        }

        usize i = 0;
        auto rand = Rand(hash(pos, min, max, global.time->ticks));
        rand.n_times(
            min, max,
            [&]() {
                const auto ref =
                    level.spawn<T>(
                        pos,
                        vec4(
                            colors[rand.next<int>(0, colors.size() - 1)],
                            1.0f));
                if (refs_out && i < refs_out->size()) {
                    (*refs_out)[i++] = ref;
                }
            });
    }

    template <typename T>
        requires std::is_base_of_v<EntityParticle, T>
    static void spawn(
        Level &level,
        const vec3 &pos,
        usize min,
        usize max,
        TileId for_tile,
        std::optional<std::span<IEntityRef<T>>> refs_out = std::nullopt) {
        return
            EntityParticle::spawn<T>(
                level, pos, min, max,
                Tiles::get()[for_tile].renderer().colors(),
                refs_out);
    }

    std::string name() const override {
        return "particle";
    }

    std::optional<AABB> aabb() const override {
        return AABB::unit().scale(1.0f / SCALE).translate(this->pos);
    }

    bool collides_with_entities() const override { return false; }

    bool check_spawn_collision() const override { return false; }

    bool allow_vertically_out_of_level() const override { return false; }

    void tick() override;

    void render(RenderContext &ctx) override;
};
