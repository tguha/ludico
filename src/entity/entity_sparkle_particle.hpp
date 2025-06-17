#pragma once

#include "entity/entity_particle.hpp"

struct EntitySparkleParticle : public EntityParticle {
    using Ref = IEntityRef<EntitySparkleParticle>;
    using Base = EntityParticle;
    using Base::Base;

    // original color particle was created with
    vec4 base_color;

    bool has_gravity() const override {
        return false;
    }

    void init() override {
        Base::init();

        this->base_color = this->color;

        auto rand = Rand(hash(id, color));
        this->ticks_to_live = 80 + rand.next(-20, 20);
        this->velocity = vec3(0.0f);
    }

    void tick() override;

    // spawns a sparkle particle on the specified entity
    static Ref spawn_on_entity(
        const Entity &e,
        const vec3 &color = vec3(1.0));
};
