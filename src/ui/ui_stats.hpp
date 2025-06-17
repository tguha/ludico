#pragma once

#include "ui/ui_hud.hpp"
#include "ui/ui_event_player_change.hpp"
#include "util/types.hpp"
#include "util/lerped.hpp"

struct UIStats
    : public UIObject,
      public UIEventHandler<UIEventPlayerChange> {
    using Base = UIObject;

    static constexpr auto
        SIZE = uvec2(80, 7),
        STAT_BAR_INTERNAL_SIZE = uvec2(32, 3),
        FILL_SIZE = uvec2(3, 3);

    static constexpr auto STAT_BAR_INTERNAL_OFFSETS =
        types::make_array(
            ivec2(6, 2),
            ivec2(42, 2));

    UIStats() : Base() {}

    UIHUD &hud() const {
        return dynamic_cast<UIHUD&>(*this->parent);
    }

    void on_event(const UIEventPlayerChange &e) override {
        UIEventHandler<UIEventPlayerChange>::on_event(e);
        this->battery_lerped = Lerped<f32>(LERP_SPEED);
        this->charge_lerped = Lerped<f32>(LERP_SPEED);
    }

    UIStats &configure(bool enabled = true);

    void reposition() override;

    void render(SpriteBatch &batch, RenderState render_state) override;

private:
    static constexpr auto LERP_SPEED = 0.025f;

    Lerped<f32>
        battery_lerped = Lerped<f32>(LERP_SPEED),
        charge_lerped = Lerped<f32>(LERP_SPEED);
};
