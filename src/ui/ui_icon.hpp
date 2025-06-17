#pragma once

#include "ui/ui_object.hpp"

struct UIIcon : public UIObject {
    using Base = UIObject;

    TextureArea icon;

    UIIcon() = default;
    explicit UIIcon(const TextureArea &icon)
        : icon(icon) {}

    UIIcon &configure(bool enabled = true) {
        Base::configure(this->icon.size(), enabled);
        return *this;
    }

    void render(SpriteBatch &batch, RenderState render_state) {
        batch.push(this->icon, this->pos_absolute(), this->color());
    }

    virtual vec4 color() const {
        return vec4(0.0f);
    }
};
