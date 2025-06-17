#pragma once

#include "util/types.hpp"
#include "util/util.hpp"
#include "util/demangle.hpp"
#include "util/math.hpp"
#include "util/string.hpp"
#include "gfx/bgfx.hpp"

// forward declaration
struct Input;
struct Window;

struct Platform {
    std::string resources_path;
    std::ostream *log_out, *log_err;
    std::unique_ptr<Window> window;
    std::unordered_map<std::string, std::unique_ptr<Input>> inputs;

    Platform();
    ~Platform();

    void update();
    void tick();

    // get inputs by name
    template <typename T>
    inline T &get_input(std::optional<std::string> name = std::nullopt) {
        if (!name) {
            auto n = util::demangle(typeid(T).name());

            if (n.find(':') != std::string::npos) {
                n = n.substr(n.rfind(':') + 1, n.length());
            }

            name = n;
        }

        return *dynamic_cast<T*>(inputs[strings::to_lower(*name)].get());
    }
};
