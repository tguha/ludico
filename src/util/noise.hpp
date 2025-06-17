#pragma once

#include "util/types.hpp"
#include "util/math.hpp"

struct Noise {
    virtual ~Noise() = default;
    virtual f32 sample(const vec2 &i) const = 0;
};

struct NoiseOctave : Noise {
    u64 seed;
    usize n;
    f32 o;

    NoiseOctave(u64 seed, usize n, f32 o) : seed(seed), n(n), o(o) {}
    f32 sample(const vec2 &i) const override;
};

struct NoiseCombined : Noise {
    std::unique_ptr<Noise> n, m;

    template <typename N, typename M>
    NoiseCombined(const N &n, const M &m)
        : n(std::make_unique<N>(n)),
          m(std::make_unique<M>(m)) {}

    f32 sample(const vec2 &i) const override;
};
