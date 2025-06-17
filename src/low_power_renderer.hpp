#pragma once

#include "gfx/util.hpp"
#include "gfx/sprite_batch.hpp"
#include "util/lerped.hpp"

struct EntityPlayer;

// TODO: generify into some screen-effect?
struct LowPowerRenderer {
    static LowPowerRenderer &instance();

    void render(
        const EntityPlayer &player,
        SpriteBatch &batch,
        RenderState render_state);

private:
    // state on [0, 1]
    Lerped<f32> state = Lerped<f32>(0.01f);
};
