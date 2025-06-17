#pragma once

#include "entity/entity_plant.hpp"
#include "model/model_daisy.hpp"
#include "locale.hpp"

struct EntityDaisy
    : public EntityPlant {
    enum Stage : u8 {
        STAGE_0 = 0,
        STAGE_1,
        STAGE_2,
        STAGE_3,
        STAGE_4,
        STAGE_COUNT
    };

    EntityDaisy();

    std::string name() const override {
        return "daisy";
    }

    std::string locale_name() const override {
        return Locale::instance()["entity:daisy"];
    }

    void render(RenderContext &ctx) override;

    f32 growth_variance() const override {
        return 0.1f;
    }

    const ModelDaisy &model() const override;

    GrowthStage num_stages() const override {
        return STAGE_COUNT;
    }

    StageDrops stage_drops() const override;
};
