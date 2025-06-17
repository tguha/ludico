#pragma once

#include "model/model_humanoid.hpp"

struct EntityShrine;

struct ModelShrine : public ModelHumanoid {
    using Base = ModelHumanoid;

    struct RenderParams {
        const EntityShrine *entity;
        const AnimationState *anim;
    };

    ModelShrine();

    void render(RenderContext &ctx, const RenderParams &params) const;
};
