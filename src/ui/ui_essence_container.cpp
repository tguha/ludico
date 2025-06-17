#include "ui/ui_essence_container.hpp"
#include "gfx/sprite_resource.hpp"
#include "essence/essence.hpp"

static constexpr auto NUM_LEVELS = 10;

static const auto spritesheet =
    SpritesheetResource(
        "ui_essence_container",
        "ui/essence_container.png",
        { 16, 16 });

void UIEssenceContainer::render(SpriteBatch &batch, RenderState render_state) {
    const auto &holder = this->holder();

    const auto
        max = holder.max,
        amount = holder.stack.amount,
        n =
            max == 0 || amount == 0 ?
            0
            : math::min<usize>(
                (amount / static_cast<f64>(max)) * NUM_LEVELS,
                NUM_LEVELS);

    batch.push(
        spritesheet[uvec2(n, 0)] .subpixels(AABB2u(SIZE - 1u)),
        this->pos_absolute());
}
