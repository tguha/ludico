#include "ui/ui_item_slot_basic.hpp"
#include "gfx/sprite_resource.hpp"
#include "global.hpp"

static const auto spritesheet =
    SpritesheetResource("ui_item_slot", "ui/item_slot.png", { 16, 16 });

void UIItemSlotBasic::render(SpriteBatch &batch, RenderState render_state) {
    const auto &sprites = spritesheet.get();
    const auto
        sprite_base =
            this->alternate.base ?
                *this->alternate.base
                : sprites[{ 0 + this->color, 0 }],
        sprite_disabled =
             this->alternate.disabled ?
                *this->alternate.disabled
                : sprites[{ 0, 1 }],
        sprite_highlight =
             this->alternate.highlight ?
                *this->alternate.highlight
                : sprites[{ 1, 1 }];

    const auto pos = this->pos_absolute();
    if (this->disabled) {
        batch.push(sprite_disabled, pos, vec4(0.0f), 1.0f, this->z);
    } else {
        batch.push(sprite_base, pos, vec4(0.0f), 1.0f, this->z);

        this->render_stack(batch, render_state);

        if (this->highlight || this->is_hovered()) {
            auto &entry = batch.push(sprite_highlight, pos);
            entry.z = Z_HIGHLIGHT;
        }

        this->render_stack_indicator(
            batch, render_state, pos + ivec2(0, -1));
    }

    // draw overlay if present
    if (this->overlay && !this->ref()) {
        batch.push(
            *this->overlay,
            this->pos_absolute()
                + (ivec2(this->size - this->overlay->pixels().size()) / 2),
            vec4(0.0f),
            1.0f,
            this->z + 1);
    }
}
