#pragma once

#include "model/model_humanoid.hpp"

struct EntityPlayer;

struct ModelPlayer : public ModelHumanoid {
    using Base = ModelHumanoid;

    struct AnimationState : public ModelHumanoid::AnimationState {

    };

    struct RenderParams {
        const EntityPlayer *entity;
        const AnimationState *anim;
    };

    const VoxelMultiMeshEntry *blinker;

    ModelPlayer();

    void render(RenderContext &ctx, const RenderParams &params) const;

    mat4 model_blinker(
        const EntityPlayer &entity,
        const AnimationState &anim,
        const mat4 &base) const;
};
