#include "util/noise.hpp"

extern "C" {
    #include <noise1234.h>
}

f32 NoiseOctave::sample(const vec2 &i) const {
    f32 u = 1.0f, v = 0.0f;
    for (usize j = 0; j < this->n; j++) {
        v += noise3(i.x / u, i.y / u, this->seed + j + (this->o * 32)) * u;
        u *= 2.0f;
    }
    return v;
}

f32 NoiseCombined::sample(const vec2 &i) const {
    const auto r = this->m->sample(i);
    return this->n->sample(vec2(i.x + r, i.y - r));
}
