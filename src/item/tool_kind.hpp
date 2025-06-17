#pragma once

#include "util/util.hpp"
#include "util/types.hpp"
#include "util/assert.hpp"

namespace ToolKind {
    enum Enum {
        HAMMER = 0,
        CLAW,
        COUNT
    };

    inline std::string to_string(Enum e) {
        static constexpr auto LOOKUP =
            types::make_array(
                "HAMMER",
                "CLAW");
        ASSERT(e < LOOKUP.size());
        return LOOKUP[e];
    }
};

