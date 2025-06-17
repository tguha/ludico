#pragma once

#include "ui/ui_button.hpp"

struct UIButtonBasic : public UIButton {
    struct Sprites {
        TextureArea base, hovered, down, disabled;
    };

    Sprites sprites;

    UIButtonBasic() = default;
    explicit UIButtonBasic(const Sprites &sprites)
        : sprites(sprites) {}

    void render(SpriteBatch &batch, RenderState render_state) override;
};
