#pragma once

#include "entity/entity_plant.hpp"
#include "model/model_hydrangea.hpp"
#include "locale.hpp"

struct EntityHydrangea
    : public EntityPlant {
    enum Stage : u8 {
        STAGE_0 = 0,
        STAGE_1,
        STAGE_2,
        STAGE_3,
        STAGE_4,
        STAGE_5,
        STAGE_6,
        STAGE_7,
        STAGE_COUNT
    };

    EntityHydrangea();

    std::string name() const override {
        return "hydrangea";
    }

    std::string locale_name() const override {
        return Locale::instance()["entity:hydrangea"];
    }

    void render(RenderContext &ctx) override;

    f32 growth_variance() const override {
        return 0.1f;
    }

    const ModelHydrangea &model() const override;

    GrowthStage num_stages() const override {
        return STAGE_COUNT;
    }

    StageDrops stage_drops() const override;
};
