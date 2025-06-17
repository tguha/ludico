#pragma once

#include "util/types.hpp"

struct Cursor {
    enum Mode {
        CURSOR_MODE_DISABLED = 0,
        CURSOR_MODE_NORMAL,
        CURSOR_MODE_HIDDEN,
    };

    virtual void set_mode(Mode mode) = 0;
};
