#pragma once

#include "ui/ui_object.hpp"
#include "essence/essence.hpp"

struct UIEssenceContainer : public UIObject {
    using Base = UIObject;

    static constexpr auto SIZE = uvec2(12, 12);

    UIEssenceContainer &configure(bool enabled = true) {
        Base::configure(SIZE, enabled);
        return *this;
    }

    void render(SpriteBatch &batch, RenderState render_state) override;

    // TODO: consider removing dummy?
    virtual EssenceHolder &holder() const {
        return const_cast<EssenceHolder&>(this->dummy);
    }

private:
    EssenceHolder dummy;
};
