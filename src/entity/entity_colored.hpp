#pragma once

#include "gfx/pixels.hpp"
#include "gfx/texture_atlas.hpp"
#include "util/collections.hpp"

struct EntityColored {
    static constexpr auto MAX_COLORS = 4;
    usize _n_colors;
    std::array<vec3, MAX_COLORS> _colors;

    EntityColored() = default;

    explicit EntityColored(const TextureArea &area) {
        const auto colors =
            collections::move_to<std::vector<vec4>>(
                pixels::colors(Pixels(area)));

        this->_n_colors =
            math::min<usize>(colors.size(), MAX_COLORS);

        auto colors_vec = std::vector<vec3>();
        std::transform(
            colors.begin(),
            colors.begin() + _n_colors,
            this->_colors.begin(),
            [](const vec4 &v) { return v.rgb(); });
    }

    // collection constructor (span, initializer_list, vector, etc.)
    template <typename C>
        requires (requires {typename C::value_type; })
    explicit EntityColored(C &&colors)
        : _n_colors(math::min<usize>(colors.size(), MAX_COLORS)) {
        std::copy_n(
            colors.begin(),
            this->_n_colors,
            this->_colors.begin());
    }

    virtual ~EntityColored() = default;

    virtual std::span<const vec3> colors() const { return this->_colors; };
};
