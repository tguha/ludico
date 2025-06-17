#pragma once

#include "entity/entity_mob.hpp"
#include "model/model_shrine.hpp"
#include "platform/platform.hpp"
#include "input/input.hpp"
#include "locale.hpp"

struct EntityShrine
    : public EntityMob {
    using Base = EntityMob;

    // tick when this shrine was activated
    u64 activated_ticks = std::numeric_limits<u64>::max();

    vec3 color;

    EntityShrine() = default;
    EntityShrine(const vec3 &color)
        : Entity(),
          EntityMob(),
          color(color) {}

    // returns true if this shrine has been activated
    inline bool activated() const {
        return this->activated_ticks <= global.time->ticks;
    }

    std::string name() const override {
        return Locale::instance()["entity:shrine"];
    }

    vec3 move(const vec3 &movement) override { return vec3(0.0f); }

    void render(RenderContext &ctx) override;

    LightArray lights() const override;

    const ModelShrine &model() const override;

    void tick() override;
};
