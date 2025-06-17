#pragma once

#include "entity/entity_smoke_particle.hpp"

struct EntityEmberParticle : public EntitySmokeParticle {
    using Base = EntitySmokeParticle;
    using Base::Base;

    LightArray lights() const override {
        auto light =
            Light(
                this->pos + vec3(0.0f, 0.05f, 0.0f),
                this->color.rgb() * 2.0f,
                2,
                false);
        light.att_quadratic = 32.0f;
        return light;
    }
};
