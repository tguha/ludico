#include "gfx/palette.hpp"

using namespace palette;

namespace palette {
    std::array<vec3, PALETTE_SIZE> PALETTE_LAB;
    std::array<vec3, PALETTE_SIZE> PALETTE_RGB;
}

void palette::palette_init() {
    static bool initialized = false;

    if (initialized) {
        return;
    }

    initialized = true;
    for (usize i = 0; i < PALETTE_SIZE; i++) {
        u32 i_rrggbb = PALETTE[i];
        const auto
            i_rgb = vec3(
                (i_rrggbb >> 16) & 0xFF,
                (i_rrggbb >> 8) & 0xFF,
                (i_rrggbb >> 0) & 0xFF) / 255.0;
        PALETTE_LAB[i] = color::rgb_to_lab(i_rgb);
        PALETTE_RGB[i] = i_rgb;
    }
}
