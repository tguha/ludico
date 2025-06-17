#pragma once

#include "util/util.hpp"
#include "util/types.hpp"

struct Debug {
    struct {
        std::string toggle;
        bool enabled = false;
    } ui_debug = { "keyboard:f12" };

    auto visualizers() {
        return types::make_array(
            &this->ui_debug);
    }

    void tick();
};
