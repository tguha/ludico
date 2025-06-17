#include "input/input.hpp"
#include "platform/platform.hpp"
#include "platform/window.hpp"
#include "gfx/renderer.hpp"
#include "global.hpp"

void Input::update() {
    for (auto &b : this->buttons) {
        b->update();
    }
}

void Input::tick() {
    for (auto &b : this->buttons) {
        b->tick();
    }
}

Mouse::Mouse() {
    this->buttons.resize(3);
    this->buttons[Index::LEFT] =
        std::make_unique<BasicInputButton>(Index::LEFT, "left");
    this->buttons[Index::RIGHT] =
        std::make_unique<BasicInputButton>(Index::RIGHT, "right");
    this->buttons[Index::MIDDLE] =
        std::make_unique<BasicInputButton>(Index::MIDDLE, "middle");
}

void Mouse::update() {
    Input::update();
    this->pos = ivec2(
        this->pos_r *
            (vec2(Renderer::get().size)
                / vec2(global.platform->window->size())));
    this->pos =
        math::clamp(
            this->pos,
            ivec2(0),
            ivec2(Renderer::get().size) - ivec2(1));
    this->pos_delta = this->pos - this->last_pos;
    this->last_pos = this->pos;
    this->scroll_delta = this->scroll - this->last_scroll;
    this->last_scroll = this->scroll;
}

void Mouse::tick() {
    Input::tick();
    this->pos_delta_tick = this->pos - this->last_pos_tick;
    this->last_pos_tick = this->pos;
    this->scroll_delta_tick = this->scroll - this->last_scroll_tick;
    this->last_scroll_tick = this->scroll;
}

InputButton *Mouse::operator[](const std::string &name) const {
    auto n = strings::to_lower(name);

    if (n == "left") {
        return this->buttons[Index::LEFT].get();
    } else if (n == "right") {
        return this->buttons[Index::RIGHT].get();
    } else if (n == "middle") {
        return this->buttons[Index::MIDDLE].get();
    }

    return nullptr;
};

