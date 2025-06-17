#include "ui/ui_pointer_label.hpp"
#include "gfx/game_camera.hpp"
#include "state/state_game.hpp"
#include "ui/cursor_attachment_item.hpp"
#include "ui/ui_item_slot.hpp"
#include "ui/ui_root.hpp"
#include "gfx/font.hpp"
#include "gfx/renderer.hpp"
#include "util/color.hpp"
#include "global.hpp"

void UIPointerLabel::reposition() {
    this->align(UIObject::ALIGN_LEFT | UIObject::ALIGN_BOTTOM);
    this->pos += SPACING;
}

void UIPointerLabel::render(SpriteBatch &batch, RenderState render_state) {
    auto &renderer = Renderer::get();

    std::string label;

    if (!this->root->has_mouse()) {
        if (const auto e = global.game->camera->mouse_entity()) {
            if (e->highlight()) {
                label = e->locale_name();
            }
        }
    }

    if (label.length() == 0) {
        label = this->fade.last();
    } else {
        this->fade.set(label);
    }

    if (this->fade.done()) {
        return;
    }

    StackAllocator<4096> allocator;
    const auto fs_label =
        renderer.font->fit(
            &allocator,
            "???",
            AABB2u(SIZE)
                .translate(uvec2(this->pos_absolute())),
            Font::ALIGN_LEFT | Font::ALIGN_BOTTOM,
            1);

    renderer.font->render(
        batch, fs_label,
        vec4(color::argb32_to_v(0xFFE6D55A).rgb(), this->fade.value()),
        Font::DOUBLED);
}
