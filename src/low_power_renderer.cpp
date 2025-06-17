#include "low_power_renderer.hpp"
#include "gfx/vignette_renderer.hpp"
#include "entity/entity_player.hpp"
#include "gfx/renderer.hpp"
#include "gfx/font.hpp"
#include "global.hpp"

LowPowerRenderer &LowPowerRenderer::instance() {
    static auto instance = LowPowerRenderer();
    return instance;
}

void LowPowerRenderer::render(
    const EntityPlayer &player,
    SpriteBatch &batch,
    RenderState render_state) {
    this->state =
        (*player.battery() < EntityPlayer::LOW_POWER_THRESHOLD) ? 1.0f : 0.0f;

    if (*this->state < math::epsilon<f32>()) {
        return;
    }

    auto &renderer = Renderer::get();
    StackAllocator<4096> allocator;

    // scale in intensity on [0, 1] according to threshold
    const auto intensity =
        math::max(
            1.0f - (*player.battery() / EntityPlayer::LOW_POWER_THRESHOLD),
            0.05f);

    VignetteRenderer::render(
        0.5f + (0.8f * intensity),
        vec4(
            1.0f, 0.0f, 0.0f,
            (0.3f + (0.7f * math::abs(math::time_wave(1.5f)))) * (*this->state)),
        render_state);

    if (math::time_wave(1.0f) > 0.0f) {
        renderer.font->render(
            batch,
            renderer.font->fit(
                &allocator,
                "LOW POWER",
                AABB2u(Renderer::get().target_size),
                Font::ALIGN_CENTER_H | Font::ALIGN_TOP,
                2),
            vec4(1.0f, 0.0f, 0.0f, *this->state),
            Font::DOUBLED);
    }
}
