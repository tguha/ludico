#include "platform/glfw/platform_glfw.hpp"
#include "util/math.hpp"
#include "util/toml.hpp"
#include "global.hpp"

#include <GLFW/glfw3.h>

// TODO: change for other platforms
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>

// TODO: better
static bool enable_errors = true;

static void error_callback(int err, const char *msg) {
    if (enable_errors) {
        ERROR(
            "GLFW ERROR ({}): {}",
            err,
            std::string_view(msg));
    }
}

void GLFWCursor::set_mode(Mode mode) {
    int glfw_mode;

    switch (mode) {
        case CURSOR_MODE_DISABLED: glfw_mode = GLFW_CURSOR_DISABLED; break;
        case CURSOR_MODE_NORMAL: glfw_mode = GLFW_CURSOR_NORMAL; break;
        case CURSOR_MODE_HIDDEN: glfw_mode = GLFW_CURSOR_HIDDEN; break;
    }

    glfwSetInputMode(
        this->window->window,
        GLFW_CURSOR,
        glfw_mode);
}

GLFWWindow::GLFWWindow(uvec2 size, const std::string &title) {
    glfwSetErrorCallback(error_callback);

    if (!glfwInit()) {
        std::cerr << "Error initializing GLFW" << std::endl;
        std::exit(1);
    }

    int num_monitors;
    GLFWmonitor **monitors = glfwGetMonitors(&num_monitors);
    GLFWmonitor *monitor = monitors[0];

    auto monitor_index =
        (*global.settings)["gfx"]["monitor"].value_or(0);

    if (monitor_index >= num_monitors) {
        WARN(
            "No such monitor {}",
            monitor_index);
    } else {
        monitor = monitors[monitor_index];
    }

    const GLFWvidmode *video_mode = glfwGetVideoMode(monitor);

    ivec2 monitor_pos;
    glfwGetMonitorPos(monitor, &monitor_pos.x, &monitor_pos.y);

    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(
        GLFW_MAXIMIZED,
        (*global.settings)["gfx"]["maximized"].value_or(false) ?
            GLFW_TRUE : GLFW_FALSE);
    this->window =
        glfwCreateWindow(
            size.x, size.y,
            title.c_str(), nullptr, nullptr);

    if (!window) {
        std::cerr << "Error creating window" << std::endl;
        glfwTerminate();
        std::exit(1);
    }

    // center and show window
    glfwDefaultWindowHints();

    const auto monitor_size = uvec2(video_mode->width, video_mode->height);
    const auto window_pos = monitor_pos + ivec2((monitor_size - size)) / 2;

    if (math::all(math::greaterThanEqual(window_pos, ivec2(0)))) {
        glfwSetWindowPos(this->window, window_pos.x, window_pos.y);
    }

    glfwSetWindowUserPointer(this->window, this);
    glfwShowWindow(this->window);

    // glfw specific, cannot handle a separate render thread
    // tell bgfx not to create a separate render thread
    bgfx::renderFrame();

    // don't trigger a reset on the first frame
    ivec2 s;
    glfwGetWindowSize(this->window, &s.x, &s.y);
    this->_size = uvec2(s);

    // initialize fields potentially requiring full GLFW initialization
    this->_cursor = GLFWCursor(this);
}

GLFWWindow::~GLFWWindow() {
    if (this->window == nullptr) {
        return;
    }

    glfwTerminate();
    std::exit(0);
}

void GLFWWindow::set_platform_data(
        bgfx::PlatformData &platform_data
    ) {
    // TODO: change for other platforms
    platform_data.nwh =
        reinterpret_cast<void *>(
            glfwGetCocoaWindow(this->window));
}

void GLFWWindow::prepare_frame() {
    glfwPollEvents();

    const auto old_size = this->_size;
    ivec2 s;
    glfwGetWindowSize(this->window, &s.x, &s.y);
    this->_size = uvec2(s);
    this->_resized = this->_size != old_size;
}

void GLFWWindow::end_frame() {

}

bool GLFWWindow::is_close_requested() {
    return glfwWindowShouldClose(this->window);
}

void GLFWWindow::close() {
    glfwSetWindowShouldClose(this->window, true);
}

bool GLFWWindow::resized() {
    return this->_resized;
}

uvec2 GLFWWindow::size() {
    return this->_size;
}

GLFWKeyboard::GLFWKeyboard(GLFWWindow &window) : Keyboard() {
    // TODO: check that another keyboard has not already been added
    window.keyboard = this;
    auto wrapper =
        [](GLFWwindow *w, int k, int s, int a, int m) {
            auto window = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(w));
            window->keyboard->callback(w, k, s, a, m);
        };
    glfwSetKeyCallback(window.window, wrapper);

    // extra names not covered by glfwGetKeyName
    static const std::unordered_map<int, std::string> names = {
        { GLFW_KEY_SPACE,         "space"         },
        { GLFW_KEY_ESCAPE,        "escape"        },
        { GLFW_KEY_ENTER,         "enter"         },
        { GLFW_KEY_TAB,           "tab"           },
        { GLFW_KEY_BACKSPACE,     "backspace"     },
        { GLFW_KEY_INSERT,        "insert"        },
        { GLFW_KEY_DELETE,        "delete"        },
        { GLFW_KEY_RIGHT,         "right"         },
        { GLFW_KEY_LEFT,          "left"          },
        { GLFW_KEY_DOWN,          "down"          },
        { GLFW_KEY_UP,            "up"            },
        { GLFW_KEY_PAGE_UP,       "page_up"       },
        { GLFW_KEY_PAGE_DOWN,     "page_down"     },
        { GLFW_KEY_HOME,          "home"          },
        { GLFW_KEY_END,           "end"           },
        { GLFW_KEY_CAPS_LOCK,     "caps_lock"     },
        { GLFW_KEY_SCROLL_LOCK,   "scroll_lock"   },
        { GLFW_KEY_NUM_LOCK,      "num_lock"      },
        { GLFW_KEY_PRINT_SCREEN,  "print_screen"  },
        { GLFW_KEY_PAUSE,         "pause"         },
        { GLFW_KEY_F1,            "f1"            },
        { GLFW_KEY_F2,            "f2"            },
        { GLFW_KEY_F3,            "f3"            },
        { GLFW_KEY_F4,            "f4"            },
        { GLFW_KEY_F5,            "f5"            },
        { GLFW_KEY_F6,            "f6"            },
        { GLFW_KEY_F7,            "f7"            },
        { GLFW_KEY_F8,            "f8"            },
        { GLFW_KEY_F9,            "f9"            },
        { GLFW_KEY_F10,           "f10"           },
        { GLFW_KEY_F11,           "f11"           },
        { GLFW_KEY_F12,           "f12"           },
        { GLFW_KEY_LEFT_SHIFT,    "left_shift"    },
        { GLFW_KEY_LEFT_CONTROL,  "left_control"  },
        { GLFW_KEY_LEFT_ALT,      "left_alt"      },
        { GLFW_KEY_LEFT_SUPER,    "left_super"    },
        { GLFW_KEY_RIGHT_SHIFT,   "right_shift"   },
        { GLFW_KEY_RIGHT_CONTROL, "right_control" },
        { GLFW_KEY_RIGHT_ALT,     "right_alt"     },
        { GLFW_KEY_RIGHT_SUPER,   "right_super"   },
    };

    // add all keys
    for (usize i = 0; i < GLFW_KEY_LAST; i++) {
        enable_errors = false;
        const auto glfw_name = glfwGetKeyName(i, 0);
        enable_errors = true;

        if (!glfw_name && !names.contains(i)) {
            continue;
        }

        const auto name =
            glfw_name ?
                strings::to_lower(glfw_name) : names.at(i);

        auto *b = new BasicInputButton(i, name);
        this->buttons.emplace_back(b);
        this->keys.emplace(name, b);
        this->keys_by_id.emplace(b->id, b);
        LOG("Registered key {}", name);
    }
}

InputButton *GLFWKeyboard::operator[](const std::string &name) const {
    const auto n = strings::to_lower(name);
    return this->keys.contains(n) ? this->keys.at(n) : nullptr;
}

void GLFWKeyboard::callback(
    UNUSED GLFWwindow *window,
    int key, int scancode, int action, UNUSED int mods) {
    if (action == GLFW_REPEAT || key < 0) {
        return;
    }

    this->keys_by_id[key]->down = action != GLFW_RELEASE;
}

GLFWMouse::GLFWMouse(GLFWWindow &window) : Mouse() {
    this->window = &window;
    window.mouse = this;

    auto wrapper_pos =
        [](GLFWwindow *w, double x, double y) {
            static_cast<GLFWWindow*>(glfwGetWindowUserPointer(w))
                ->mouse->callback_pos(w, x, y);
        };

    auto wrapper_button =
        [](GLFWwindow *w, int b, int a, int m) {
            static_cast<GLFWWindow*>(glfwGetWindowUserPointer(w))
                ->mouse->callback_button(w, b, a, m);
        };

    auto wrapper_enter =
        [](GLFWwindow *w, int e) {
            static_cast<GLFWWindow*>(glfwGetWindowUserPointer(w))
                ->mouse->callback_enter(w, e);
        };

    auto wrapper_scroll =
        [](GLFWwindow *w, double x, double y) {
            static_cast<GLFWWindow*>(glfwGetWindowUserPointer(w))
                ->mouse->callback_scroll(w, x, y);
        };

    glfwSetCursorPosCallback(window.window, wrapper_pos);
    glfwSetMouseButtonCallback(window.window, wrapper_button);
    glfwSetCursorEnterCallback(window.window, wrapper_enter);
    glfwSetScrollCallback(window.window, wrapper_scroll);
}

void GLFWMouse::set_mode(GLFWMouse::Mode mode) {
    this->mode = mode;

    switch (mode) {
        case GLFWMouse::DISABLED:
            glfwSetInputMode(
                this->window->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            break;
        case GLFWMouse::HIDDEN:
            glfwSetInputMode(
                this->window->window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
            break;
        case GLFWMouse::NORMAL:
            glfwSetInputMode(
                this->window->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            break;
    }
}

void GLFWMouse::callback_pos(
    UNUSED GLFWwindow *window, double x, double y) {
    // NOTE: position is flipped! (0, 0) is bottom left
    this->pos_r = vec2(x, this->window->size().y - y);
}

void GLFWMouse::callback_button(
    UNUSED GLFWwindow *window, int button, int action, UNUSED int mods) {
    InputButton *b = nullptr;

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        b = (*this)["left"];
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        b = (*this)["right"];
    } else if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
        b = (*this)["middle"];
    }

    if (b) {
        dynamic_cast<BasicInputButton *>(b)->down = action == GLFW_PRESS;
    }
}

void GLFWMouse::callback_enter(
    UNUSED GLFWwindow *window, int entered) {
    this->in_window = static_cast<bool>(entered);
}

void GLFWMouse::callback_scroll(
    UNUSED GLFWwindow *window, UNUSED double x, double y) {
    const auto sensitivity =
        (*global.settings)["mouse"]["scroll_sensitivity"].value_or(1.0f);
    this->scroll += y * sensitivity;
}

