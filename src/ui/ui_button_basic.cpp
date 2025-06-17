#include "ui/ui_button_basic.hpp"

void UIButtonBasic::render(SpriteBatch &batch, RenderState render_state) {
    TextureArea sprite = this->sprites.base;

    if (!this->is_enabled()) {
        sprite = this->sprites.disabled;
    } else if (this->is_down()) {
        sprite = this->sprites.down;
    } else if (this->is_hovered()) {
        sprite = this->sprites.hovered;
    }

    batch.push(sprite, this->pos_absolute());
}
