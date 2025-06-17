#include "util/math.hpp"
#include "global.hpp"
#include "util/time.hpp"

f32 math::time_wave(f32 period_seconds, f32 offset) {
    const auto period_ticks = period_seconds * TICKS_PER_SECOND;
    return math::sin(
        (math::TAU
            * (math::mod(f32(global.time->ticks), period_ticks)
                / period_ticks)) + offset);
}
