#pragma once

#include "ui/ui_fade_helper.hpp"
#include "ui/ui_object.hpp"

struct UIPointerLabel : public UIObject {
    using Base = UIObject;

    static constexpr auto
        SIZE = uvec2(104, 64),
        SPACING = uvec2(2, 2);

    UIPointerLabel() = default;

    UIPointerLabel &configure(bool enabled = true) {
        Base::configure(SIZE, enabled);
        this->reposition();
        return *this;
    }

    bool is_ghost() const override { return true; }

    void reposition() override;

    void render(SpriteBatch &batch, RenderState render_state) override;

private:
    UIFadeHelper<std::string> fade = UIFadeHelper<std::string>(80);
};
