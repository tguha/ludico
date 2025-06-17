#pragma once

#include "entity/entity_particle.hpp"

struct EntitySmashParticle : public EntityParticle {
    using Base = EntityParticle;
    using Base::Base;

    void init() override {
        Base::init();

        auto rand = Rand(hash(id, color));

        this->ticks_to_live = TICKS_PER_SECOND + rand.next(-30, 30);

        this->drag = 1.05f;
        this->restitution = 1.02f;

        constexpr auto r = 0.08f;
        this->velocity =
            vec3(
                rand.next(-r, r),
                rand.next(0.05f, 0.09f),
                rand.next(-r, r));
    }
};
