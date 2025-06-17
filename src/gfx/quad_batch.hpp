#pragma once

#include "util/list.hpp"
#include "gfx/texture.hpp"
#include "gfx/vertex.hpp"

struct RenderState;

struct QuadBatch {
    struct Entry {
        ivec2 pos;
        uvec2 size;
        vec4 color;
    };

    List<Entry> quads;

    explicit QuadBatch(Allocator *allocator, usize reserved = 32)
        : quads(allocator) {
        this->quads.reserve(reserved);
    }

    void push(
        const ivec2 &pos,
        const uvec2 &size,
        const vec4 &color = vec4(1.0)) {
        this->quads.push(Entry { pos, size, color });
    }

    void push_outline(
        const ivec2 &pos,
        const uvec2 &size,
        const vec4 &color = vec4(1.0)) {
        // bottom left -> top left
        this->quads.push(
            Entry { pos, uvec2(1, size.y), color });
        // bottom left -> bottom right
        this->quads.push(
            Entry { pos + ivec2(1, 0), uvec2(size.x - 1, 1), color });
        // top left -> top right
        this->quads.push(
            Entry { pos + ivec2(0, size.y - 1), uvec2(size.x, 1), color });
        // bottom right -> top right
        this->quads.push(
            Entry { pos + ivec2(size.x - 1, 0), uvec2(1, size.y - 1), color });
    }

    void render(RenderState render_state) const;
};
