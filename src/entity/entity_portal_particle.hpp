#pragma once

#include "entity/entity_particle.hpp"

struct EntityPortalParticle : public EntityParticle {
    using Base = EntityParticle;

    static constexpr auto RADIUS = 0.01f;

    using Base::Base;

    bool has_gravity() const override { return false; }

    void init() override {
        Base::init();
        auto rand = Rand(hash(this->id));
        this->drag = 1.005f;

        this->velocity =
            vec3(
                rand.next(-RADIUS, RADIUS),
                rand.next(0.003f, 0.008f),
                rand.next(-RADIUS, RADIUS));
        this->ticks_to_live =
            TICKS_PER_SECOND * 3 + rand.next(-60, 60);
    }

    void tick() override {
        Base::tick();

        auto rand = Rand(hash(global.time->ticks, this->id));
        const auto
            s = 120.0f,
            t = (math::TAU * ((global.time->ticks % u64(s)) / f32(s))) + this->id,
            a = 0.04f + rand.next(-0.01f, 0.01f);

        this->velocity +=
            vec3(
                a * math::sin(t) * RADIUS,
                0.0f,
                a * math::cos(t) * RADIUS);
    }

    LightArray lights() const override {
        auto light =
            Light(
                this->pos + vec3(0.0f, 0.05f, 0.0f),
                this->color.rgb() * 1.5f,
                2,
                false);
        light.att_quadratic = 32.0f;
        return light;
    }
};
