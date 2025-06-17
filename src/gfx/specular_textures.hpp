#pragma once

#include "gfx/texture_atlas.hpp"

// manages a set of specular textures with evenly spaced values in [0.0f, 1.0f]
// for entities/tiles/etc. which do not have a custom specular texture
struct SpecularTextures {
    static constexpr auto COUNT = 17;
    static_assert(
        math::cexp::popcnt(COUNT - 1) == 1,
        "COUNT must be power-of-two + 1");

    SpecularTextures();
    SpecularTextures(const SpecularTextures&) = delete;
    SpecularTextures(SpecularTextures&&) = default;
    SpecularTextures &operator=(const SpecularTextures&) = delete;
    SpecularTextures &operator=(SpecularTextures&&) = default;

    inline auto begin() const { return this->areas.begin(); }
    inline auto end () const { return this->areas.end(); }
    inline auto begin() { return this->areas.begin(); }
    inline auto end () { return this->areas.end(); }

    inline auto &operator[](const auto &i) const { return this->areas[i]; }
    inline auto &operator[](const auto &i) { return this->areas[i]; }

    inline auto size() { return this->areas.size(); }

    inline TextureArea max() const { return (*this)[COUNT - 1]; }
    inline TextureArea min() const { return (*this)[0]; }

    // s is clamped into [0.0, 1.0]
    inline TextureArea closest(f32 s) const {
        constexpr usize M = 256 / (COUNT - 1);
        const auto i =
            math::roundMultiple(
                usize(math::clamp(s, 0.0f, 1.0f) * 256),
                M) / M;
        return (*this)[i];
    }

    static const SpecularTextures &instance();

private:
    std::array<TextureArea, COUNT> areas;
};
