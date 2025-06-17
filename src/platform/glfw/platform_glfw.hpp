#pragma once

#include "util/types.hpp"
#include "util/util.hpp"
#include "platform/platform.hpp"
#include "platform/window.hpp"
#include "platform/cursor.hpp"
#include "input/input.hpp"

struct GLFWwindow;
struct GLFWKeyboard;
struct GLFWMouse;
struct GLFWWindow;

struct GLFWCursor : public Cursor {
    GLFWWindow *window;

    GLFWCursor() = default;
    GLFWCursor(GLFWWindow *window)
        : window(window) {}
    void set_mode(Mode mode) override;
};

struct GLFWWindow : public Window {
    GLFWwindow *window;
    GLFWKeyboard *keyboard;
    GLFWMouse *mouse;

    GLFWWindow() = default;
    GLFWWindow(uvec2 size, const std::string& title);
    ~GLFWWindow() override;

    GLFWWindow(GLFWWindow const &other) : window(other.window) {}

    GLFWWindow(GLFWWindow &&other) : window(other.window) {
        other.window = nullptr;
    }

    GLFWWindow &operator=(GLFWWindow const &other) {
        return *this = GLFWWindow(other);
    }

    GLFWWindow &operator=(GLFWWindow &&other) {
        this->window = other.window;
        other.window = nullptr;
        return *this;
    }

    void set_platform_data(bgfx::PlatformData &platform_data) override;
    void prepare_frame() override;
    void end_frame() override;
    bool is_close_requested() override;
    void close() override;
    bool resized() override;
    uvec2 size() override;

    std::optional<Cursor*> cursor() override {
        return static_cast<Cursor*>(&this->_cursor);
    }

private:
    GLFWCursor _cursor;
    uvec2 _size;
    bool _resized;
};

// TODO: shift/"modifier" keys do not fully work. implement these by having
// GLFWKeyboard::update use glfwGetKey with <LEFT/RIGHT>_SHIFT and manually
// updating the values each frame
struct GLFWKeyboard final
    : public Keyboard {
    // keys is originally this size and *must not* be changed
    static const constexpr usize MAX_KEYS = 1024;

    std::unordered_map<std::string, BasicInputButton*> keys;
    std::unordered_map<int, BasicInputButton*> keys_by_id;

    GLFWKeyboard(GLFWWindow &window);
    InputButton *operator[](const std::string &name) const override;
private:
    void callback(
        GLFWwindow *window, int key, int scancode, int action, int mods);
};

struct GLFWMouse final
    : public Mouse {
    GLFWWindow *window;
    GLFWMouse(GLFWWindow &window);
    void set_mode(GLFWMouse::Mode mode) override;
private:
    void callback_pos(
        GLFWwindow *window, double x, double y);
    void callback_button(
        GLFWwindow *window, int button, int action, int mods);
    void callback_enter(
        GLFWwindow *window, int entered);
    void callback_scroll(
        GLFWwindow *window, double x, double y);
};
