#pragma once

#include "entity/entity_plant.hpp"
#include "model/model_sunflower.hpp"
#include "locale.hpp"

struct EntitySunflower
    : public EntityPlant {
    enum Stage : u8 {
        STAGE_0 = 0,
        STAGE_1,
        STAGE_2,
        STAGE_3,
        STAGE_4,
        STAGE_COUNT
    };

    EntitySunflower();

    std::string name() const override {
        return "sunflower";
    }

    std::string locale_name() const override {
        return Locale::instance()["entity:sunflower"];
    }

    void render(RenderContext &ctx) override;

    f32 growth_variance() const override {
        return 0.1f;
    }

    const ModelSunflower &model() const override;

    GrowthStage num_stages() const override {
        return STAGE_COUNT;
    }

    StageDrops stage_drops() const override;
};
