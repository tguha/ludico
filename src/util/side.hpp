#pragma once


namespace Side {
    enum Enum {
        LEFT = 0,
        RIGHT = 1,
        COUNT = (RIGHT + 1)
    };

    static constexpr Enum ALL[] = { LEFT, RIGHT };
}
