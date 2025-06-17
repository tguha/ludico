#pragma once

#include "util/types.hpp"
#include "util/math.hpp"
#include "util/util.hpp"
#include "util/stack_allocator.hpp"
#include "gfx/texture_atlas.hpp"
#include "gfx/sprite_batch.hpp"

struct Font {
    enum Flags {
        DOUBLED = 1 << 0
    };

    enum Alignment {
        ALIGN_CENTER_H = 1 << 0,
        ALIGN_CENTER_V = 1 << 1,
        ALIGN_LEFT = 1 << 2,
        ALIGN_RIGHT = 1 << 3,
        ALIGN_TOP = 1 << 4,
        ALIGN_BOTTOM = 1 << 5
    };

    struct PosString {
        std::string str;
        ivec2 pos;
    };

    static constexpr auto GLYPH_SIZE = uvec2(8, 8);

    void init();

    usize width(const std::string &str) const {
        return str.length() * GLYPH_SIZE.x;
    }

    usize height() const {
        return GLYPH_SIZE.y;
    }

    usize lines(const std::string &str) const {
        return std::count(str.begin(), str.end(), '\n');
    }

    void render(
        SpriteBatch &dst,
        const ivec2 &pos,
        char c,
        const vec4 &color = vec4(1.0),
        u64 flags = 0,
        isize z = SpriteBatch::Z_DEFAULT) const;

    void render(
        SpriteBatch &dst,
        const ivec2 &pos,
        const std::string &str,
        const vec4 &color = vec4(1.0),
        u64 flags = 0,
        isize z = SpriteBatch::Z_DEFAULT) const;

    inline void render(
        const ivec2 &pos,
        const std::string &str,
        const vec4 &color,
        RenderState render_state,
        u64 flags = 0,
        isize z = SpriteBatch::Z_DEFAULT) const {
        StackAllocator<4096> allocator;
        SpriteBatch batch(
            &allocator,
            str.length() * ((flags & DOUBLED) ? 2 : 1));
        this->render(batch, pos, str, color, flags, z);
        batch.render(render_state);
    }

    inline void render(
        SpriteBatch &dst,
        std::span<const PosString> offset_strings,
        const vec4 &color,
        u64 flags = 0,
        isize z = SpriteBatch::Z_DEFAULT) const {
        for (const auto &os : offset_strings) {
            this->render(
                dst, os.pos, os.str, color, flags, z);
        }
    }

    // splits a string into words inside of the specified box with the requested
    // alignment
    // NOTE: even though box.min is bottom left and box.max is top right,
    // strings will be fit from top to bottom
    List<PosString> fit(
        Allocator *allocator,
        const std::string &str,
        const AABB2u &box,
        usize alignment,
        usize spacing);

private:
    std::unordered_map<char, TextureArea> glyphs;
};
