#pragma once

#include "ui/ui_button_basic.hpp"

struct UIButtonArrow : public UIButtonBasic {
    using Base = UIButtonBasic;

    enum Type {
        UP = 0,
        DOWN,
        RIGHT,
        LEFT
    };

    Type type;

    explicit UIButtonArrow(Type type);

    UIButtonArrow &configure(bool enabled = true) {
        Base::configure(this->size, enabled);
        return *this;
    }

    // get size of button of specified type
    // NOTE: values must match sprites
    static constexpr uvec2 size_of(Type type) {
        return
            (type == UP || type == DOWN) ?
                uvec2(12, 7)
                : uvec2(7, 12);
    }
};
