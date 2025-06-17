#pragma once

#include "entity/entity_particle.hpp"

struct EntitySmokeParticle : public EntityParticle {
    using Base = EntityParticle;
    using Base::Base;

    bool has_gravity() const override { return false; }

    void init() override {
        Base::init();
        auto rand = Rand(hash(id, color));

        this->ticks_to_live =
            (TICKS_PER_SECOND * 3) + rand.next(-60, 60);

        this->drag = 1.005f;

        constexpr auto r = 0.01f;
        this->velocity =
            vec3(
                rand.next(-r, r),
                rand.next(0.003f, 0.08f),
                rand.next(-r, r));
    }
};
