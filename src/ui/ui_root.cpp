#include "ui/ui_root.hpp"
#include "gfx/sprite_resource.hpp"
#include "gfx/renderer.hpp"
#include "input/input.hpp"
#include "platform/platform.hpp"
#include "global.hpp"

static const auto spritesheet_cursor =
    SpritesheetResource("ui_cursor", "misc/cursor.png", { 16, 16 });

UIRoot::UIRoot() : Base() {
    this->id = 0;
    this->root = this;
    this->renderer.init();
}

UIRoot &UIRoot::configure() {
    Base::configure(Renderer::get().target_size, true);
    return *this;
}

void UIRoot::reposition() {
    this->size = Renderer::get().target_size;
    Base::reposition();
}

ivec2 UIRoot::cursor_pos() const {
    return global.platform->get_input<Mouse>().pos;
}

InputButton *UIRoot::click_button() const {
    return global.platform->get_input<Mouse>()["left"];
}

void UIRoot::render(SpriteBatch &batch, RenderState render_state) {
    Base::render(batch, render_state);

    // render current cursor attachment over everything if it exists
    if (this->cursor_attachment) {
        this->cursor_attachment->render(
            global.platform->get_input<Mouse>().pos,
            batch,
            render_state);
    }

    // draw cursor
    const auto &mouse = global.platform->get_input<Mouse>();
    auto &entry =
        batch.push(
            spritesheet_cursor[{ 0, 0 }],
            mouse.pos - ivec2(0, 16));
    entry.z = Z_MOUSE;
}

void UIRoot::tick() {
    Base::tick();

    const auto cursor_pos = this->cursor_pos();
    const auto
        top = this->topmost(cursor_pos, true),
        top_rec = this->topmost_recursive(cursor_pos, true);
    this->_has_mouse =
        (top_rec && !top_rec->is_ghost()) || this->cursor_attachment;

    const auto *cb = this->click_button();

    if (top) {
        const auto mouse_rel = cursor_pos - top->pos_absolute();
        top->hover(mouse_rel);

        if (cb->is_pressed_tick()) {
            top->click(mouse_rel, false);
        } else if (cb->is_released_tick()) {
            top->click(mouse_rel, true);
        }
    }

    // find mouse object, cannot be ghost (mouse passes through)
    this->mouse_object = top_rec ? top_rec : top;

    if (this->mouse_object && this->mouse_object->is_ghost()) {
        this->mouse_object = nullptr;
    }

    if (this->cursor_attachment) {
        this->cursor_attachment->tick(cursor_pos);
    }

    // cursor attachment could have been deleted in tick, check again
    if (this->cursor_attachment && cb->is_released_tick()) {
        this->cursor_attachment->on_click(true, mouse_object);
    }
}
