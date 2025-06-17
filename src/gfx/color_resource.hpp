#pragma once

#include "gfx/pixels.hpp"
#include "gfx/texture_atlas.hpp"
#include "util/collections.hpp"
#include "util/managed_resource.hpp"
#include "util/types.hpp"
#include "util/math.hpp"
#include "util/util.hpp"

struct ColorResource
    : public ValueManagedResource<std::vector<vec3>> {
    using Base = ValueManagedResource<std::vector<vec3>>;

    using SourceFn = std::function<TextureArea()>;

    explicit ColorResource(SourceFn &&source_fn)
        : Base(
            [source_fn = std::move(source_fn)]() {
                return collections::move_to<std::vector<vec3>>(
                    pixels::colors(Pixels(source_fn())),
                    [](const vec4 &v) { return v.rgb(); });
            }) {}

    void attach() override {}
};
