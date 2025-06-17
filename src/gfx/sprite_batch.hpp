#pragma once

#include "util/types.hpp"
#include "util/util.hpp"
#include "util/list.hpp"
#include "gfx/texture_atlas.hpp"

struct SpriteBatch {
    static constexpr auto
        Z_DEFAULT = 0;

    struct Entry {
        // sprite
        TextureArea area;

        // position
        ivec2 pos;

        // color, mixed with texture color according to color.a
        vec4 color;

        // overall transparency
        f32 alpha;

        // entry Z coordinate
        isize z;

        // original insertion index
        usize i;
    };

    mutable List<Entry> sprites;

    explicit SpriteBatch(Allocator *allocator, usize reserved = 32)
        : sprites(allocator) {
        this->sprites.reserve(reserved);
    }

    Entry &push(
        const TextureArea &area,
        const ivec2 &pos,
        const vec4 &color = vec4(0.0),
        f32 alpha = 1.0f,
        isize z = Z_DEFAULT) {
            this->sprites.push(
                Entry { area, pos, color, alpha, z, this->sprites.size() - 1 });
            return this->sprites[this->sprites.size() - 1];
    }

    void render(RenderState render_state) const;
};
