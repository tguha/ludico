#pragma once

#include "util/types.hpp"
#include "util/util.hpp"
#include "util/math.hpp"
#include "global.hpp"

struct InputButton {
    usize id;

    InputButton() = default;
    explicit InputButton(usize id) : id(id) {}
    virtual ~InputButton() = default;

    InputButton(const InputButton&) = delete;
    InputButton(InputButton&&) = default;
    InputButton &operator=(const InputButton&) = delete;
    InputButton &operator=(InputButton&&) = default;

    virtual void update() {}
    virtual void tick() {}

    virtual u64 tick_down() const { return std::numeric_limits<u64>::max(); }
    virtual bool is_down() const { return false; }
    virtual bool is_pressed() const { return false; }
    virtual bool is_pressed_tick() const { return false; }
    virtual bool is_released() const { return false; }
    virtual bool is_released_tick() const { return false; }
};

struct BasicInputButton : public InputButton {
    std::string name;
    bool down = false,
         last = false,
         last_tick = false,
         pressed = false,
         pressed_tick = false,
         released = false,
         released_tick = false;

    BasicInputButton() = default;
    BasicInputButton(usize id, const std::string &name)
        : InputButton(id), name(name) { }

    void update() override {
        this->pressed = this->down && !this->last;
        this->released = this->last && !this->down;
        this->last = this->down;
    }

    void tick() override {
        this->pressed_tick = this->down && !this->last_tick;
        this->released_tick = this->last_tick && !this->down;
        this->last_tick = this->down;

        if (this->pressed_tick) {
            this->_tick_down = global.time->ticks;
        } else if (!this->down) {
            this->_tick_down = std::numeric_limits<u64>::max();
        }
    }


    u64 tick_down() const override { return this->_tick_down; }
    bool is_down() const override { return this->down; }
    bool is_pressed() const override { return this->pressed; }
    bool is_pressed_tick() const override { return this->pressed_tick; }
    bool is_released() const override { return this->released; }
    bool is_released_tick() const override { return this->released_tick; }

private:
    u64 _tick_down;
};

struct CompoundInputButton : public InputButton {
    std::vector<InputButton*> buttons;

    CompoundInputButton() = default;
    explicit CompoundInputButton(InputButton *b)
        : buttons({ b }) {}
    explicit CompoundInputButton(const std::vector<InputButton*> &bs)
        : buttons(bs) {}

    void update() override {}
    void tick() override {}

    u64 tick_down() const override {
        return (**std::min_element(
            this->buttons.begin(),
            this->buttons.end(),
            [](auto &a, auto &b) { return a->tick_down() < b->tick_down(); }))
                .tick_down();
    }

    bool is_down() const override {
        return std::any_of(
            this->buttons.begin(),
            this->buttons.end(),
            [](auto &b) { return b->is_down(); });
    }

    bool is_pressed() const override {
        return std::any_of(
            this->buttons.begin(),
            this->buttons.end(),
            [](auto &b) { return b->is_pressed(); });
    }

    bool is_pressed_tick() const override {
        return std::any_of(
            this->buttons.begin(),
            this->buttons.end(),
            [](auto &b) { return b->is_pressed_tick(); });
    }

    bool is_released() const override {
        return std::any_of(
            this->buttons.begin(),
            this->buttons.end(),
            [](auto &b) { return b->is_released(); });
    }

    bool is_released_tick() const override {
        return std::any_of(
            this->buttons.begin(),
            this->buttons.end(),
            [](auto &b) { return b->is_released_tick(); });
    }
};

struct Input {
    std::vector<std::unique_ptr<InputButton>> buttons;

    virtual ~Input() = default;
    virtual void update();
    virtual void tick();

    // retrieve buttons by name
    virtual InputButton *operator[](const std::string &name) const = 0;
};

// TODO: proper text input (on GLFW, see glfwSetCharCallback)
struct Keyboard
    : Input {
};

struct Mouse
    : Input {
    enum Mode {
        DISABLED,
        HIDDEN,
        NORMAL
    };

    enum Index {
        LEFT = 0,
        RIGHT = 1,
        MIDDLE = 2
    };

    Mode mode;

    // raw mouse position
    vec2 pos_r;

    // mouse position in renderer pixel space
    ivec2 pos, pos_delta, pos_delta_tick;

    // scroll distance
    f32 scroll, scroll_delta, scroll_delta_tick;

    // true when mouse is contained in window
    bool in_window;

    virtual void set_mode(Mode mode) = 0;

    Mouse();
    void update() override;
    void tick() override;

    // built-in access for left/right/middle
    InputButton *operator[](const std::string &name) const override;
private:
    ivec2 last_pos, last_pos_tick;
    f32 last_scroll, last_scroll_tick;
};
