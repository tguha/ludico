#pragma once

#include "util/types.hpp"
#include "util/util.hpp"
#include "gfx/texture_atlas.hpp"

struct IconRenderer {
    struct PendingIcon {
        InplaceRenderFn render_fn;
        std::string label;
        uvec2 size;

        PendingIcon() = default;
        PendingIcon(
            InplaceRenderFn &&render_fn,
            const std::string &label,
            uvec2 size)
            : render_fn(std::move(render_fn)),
              label(label),
              size(size) {}
    };

    std::queue<std::unique_ptr<PendingIcon>>
        unrendered,
        pending;

    IconRenderer() = default;

    void add(
        const std::string &label,
        const uvec2 &size,
        InplaceRenderFn &&render_fn);

    std::optional<const TextureAtlasEntry*> get(
        const std::string &label) const;

    void update();
};
