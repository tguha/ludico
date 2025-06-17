#pragma once

#include "tile/tile.hpp"
#include "tile/tile_renderer_voxel.hpp"
#include "entity/entity_smoke_particle.hpp"
#include "entity/entity_ember_particle.hpp"
#include "level/level.hpp"
#include "util/time.hpp"
#include "util/hash.hpp"
#include "util/rand.hpp"
#include "constants.hpp"

struct TileTorch : public Tile {
    static constexpr auto
        SMOKE_COLOR = vec3(0.2f),
        EMBER_COLOR = vec3(0.8f, 0.65f, 0.3f);

    explicit TileTorch(TileId id);

    std::optional<AABB> aabb(
        const Level &level,
        const ivec3 &pos) const override {
        return this->aabb_base.translate(vec3(pos));
    }

    void random_tick(
        Level &level,
        const ivec3 &pos) const override {
        TileTorch::emit_particles(level, Level::to_tile_center(pos));
    }

    bool can_emit_light() const override {
        return true;
    }

    LightArray lights(
        const Level &level,
        const ivec3 &pos) const override {
        auto l = TileTorch::light();
        l.pos = Level::to_tile_center(pos);
        return l;
    }

    const TileRenderer &renderer() const override {
        return this->_renderer;
    }

    Transparency transparency_type() const override {
        return Transparency::ON;
    }

    static void emit_particles(
        Level &level,
        const vec3 &pos) {
        auto rand = Rand(hash(global.time->ticks, pos));
        if (rand.next<usize>(0, 4)) {
            EntityParticle::spawn<EntitySmokeParticle>(
                level,
                pos,
                SMOKE_COLOR);
        } else {
            EntityParticle::spawn<EntityEmberParticle>(
                level,
                pos,
                EMBER_COLOR);
        }
    }

    static Light light() {
        return Light(1.5f * vec3(1.0f, 0.83f, 0.6f), LIGHT_MAX_VALUE);
    }

private:
    TileRendererVoxelBasic _renderer;
    AABB aabb_base;
};
