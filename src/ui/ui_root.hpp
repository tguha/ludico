#pragma once

#include "ui/ui_container.hpp"
#include "ui/ui_renderer.hpp"
#include "ui/cursor_attachment.hpp"

struct InputButton;

struct UIRoot : public UIContainer {
    using Base = UIContainer;

    static constexpr auto
        Z_MOUSE = SpriteBatch::Z_DEFAULT + 100;

    // base UI renderer
    UIRenderer renderer;

    // current hovered ui object, nullptr if not present
    UIObject *mouse_object;

    // current attached cursor object, nullptr if not present
    std::unique_ptr<CursorAttachment> cursor_attachment;

    UIRoot();

    UIRoot &configure();

    void reposition() override;

    // true if the mouse is currently over a UI element
    inline bool has_mouse() const { return this->_has_mouse; }

    // get UI cursor position
    ivec2 cursor_pos() const;

    // get button used for click events
    InputButton *click_button() const;

    bool is_ghost() const override { return true; }

    void render(SpriteBatch &batch, RenderState render_state) override;

    void tick() override;

    u64 next_id() {
        return this->_next_id++;
    }

private:
    bool _has_mouse;
    u64 _next_id = 1;
};
