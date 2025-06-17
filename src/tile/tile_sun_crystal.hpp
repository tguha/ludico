#pragma once

#include "tile/tile_ore.hpp"
#include "entity/entity_portal_particle.hpp"
#include "util/rand.hpp"
#include "util/hash.hpp"
#include "level/level.hpp"
#include "constants.hpp"

struct TileSunCrystal : public TileOre {
    using Base = TileOre;
    using Base::Base;

    std::string name() const override {
        return "sun_crystal";
    }

    std::string locale_name() const override {
        return Locale::instance()["tile:sun_crystal"];
    }

    TextureArea coords_tex() const override;

    TextureArea coords_specular() const override;

    bool can_emit_light() const override {
        return true;
    }

    LightArray lights(
        const Level &level,
        const ivec3 &pos) const override {
        const auto offset = Rand(hash(pos)).next(0.0f, 256.0f);
        return Light(
            Level::to_tile_center(pos),
            vec3(0.7f) + (0.2f * math::time_wave(1.8f, offset)),
            LIGHT_MAX_VALUE);
    }

    void random_tick(
        Level &level,
        const ivec3 &pos) const override {
        Base::random_tick(level, pos);
        EntityParticle::spawn<EntityPortalParticle>(
            level,
            Level::to_tile_center(pos), 2, 4,
            *this);
    }
};
