#pragma once

#include "util/types.hpp"
#include "util/util.hpp"
#include "input/input.hpp"

struct Controls {
    CompoundInputButton
        primary,
        secondary,
        up,
        down,
        left,
        right,
        jump,
        drop,
        inventory;

    Controls() = default;

    // get input button from control string
    static CompoundInputButton from_string(const std::string &str);

    // read controls from settings
    static Controls from_settings();
};
