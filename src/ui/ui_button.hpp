#pragma once

#include "ui/ui_object.hpp"
#include "util/time.hpp"
#include "global.hpp"

struct UIButton : public UIObject {
    using CallbackFn = std::function<void(UIButton&)>;

    std::optional<CallbackFn> on_press, on_release;

    bool click(const ivec2 &pos, bool is_release) override {
        if (!this->is_enabled()) {
            return false;
        }

        if (is_release) {
            this->release_tick = global.time->ticks;

            if (this->on_release) {
                (*this->on_release)(*this);
            }
        } else {
            this->press_tick = global.time->ticks;

            if (this->on_press) {
                (*this->on_press)(*this);
            }
        }

        return true;
    }

    virtual bool is_down() {
        return this->is_enabled()
            && this->press_tick != Time::TICK_MAX
            && this->press_tick < global.time->ticks
            && (this->release_tick == Time::TICK_MAX
                || this->press_tick > this->release_tick);
    }

    virtual bool is_pressed() {
        return this->is_enabled()
            && this->press_tick != Time::TICK_MAX
            && this->press_tick == global.time->ticks;
    }

    virtual bool is_released() {
        return this->is_enabled()
            && this->release_tick != Time::TICK_MAX
            && this->release_tick == global.time->ticks;
    }

private:
    usize press_tick = Time::TICK_MAX, release_tick = Time::TICK_MAX;
};
