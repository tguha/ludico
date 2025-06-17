#pragma once

#include "gfx/texture_atlas.hpp"
#include "gfx/sprite_batch.hpp"

struct UITileset {
    enum Indices {
        CORNER_BL = 0,
        EDGE_BOTTOM,
        CORNER_BR,
        EDGE_LEFT,
        CENTER,
        EDGE_RIGHT,
        CORNER_TL,
        EDGE_TOP,
        CORNER_TR,

        INDEX_COUNT = (CORNER_TR + 1)
    };

    std::array<TextureArea, INDEX_COUNT> tiles;

    inline auto &operator[](usize i) {
        ASSERT(i < INDEX_COUNT, fmt::format("{}", i));
        return this->tiles[i];
    }

    inline auto &operator[](usize i) const {
        ASSERT(i < INDEX_COUNT, fmt::format("{}", i));
        return this->tiles[i];
    }

    inline uvec2 tile_size() const {
        const auto res = tiles[0].pixels().size();
        ASSERT(res != uvec2(0));
        return res;
    }

    static inline UITileset from(
        const TextureAtlasEntry &entry,
        const uvec2 &offset) {
        UITileset result;
        for (usize y = 0; y < 3; y++) {
            for (usize x = 0; x < 3; x++) {
                result[(y * 3) + x] = entry[offset + uvec2(x, y)];
            }
        }
        return result;
    }
};

struct UIRenderer {
    enum Tileset {
        TS_BASE = 0,
        TS_BUTTON_BASE,
        TS_BUTTON_PRESS,

        TS_COUNT = (TS_BUTTON_PRESS + 1)
    };

    struct Tilesets {
        std::array<UITileset, TS_COUNT> data;

        void init();

        inline auto &operator[](Tileset t) const {
            return this->data.at(t);
        }

        inline auto &operator[](Tileset t) {
            return this->data[t];
        }
    };

    Tilesets tilesets;

    void init();

    void render_background(
        SpriteBatch &dest,
        const UITileset &tileset,
        const ivec2 &pos,
        const uvec2 &size,
        const vec4 &color = vec4(0.0)) const;

    void render_tile_fill(
        SpriteBatch &dest,
        const TextureArea &tile,
        const ivec2 &pos,
        const uvec2 &size,
        const vec4 &color = vec4(0.0)) const;

    struct BarTileset {
        union {
            struct {
                TextureArea left, center, right, partial, end;
            };

            std::array<TextureArea, 5> textures;
        };

        static inline BarTileset from(
            const MultiTexture &texture, const uvec2 &offset) {
            BarTileset result;
            for (usize i = 0; i < 5; i++) {
                result.textures[i] = texture[offset + uvec2(i, 0)];
            }
            return result;
        }
    };

    void render_bar(
        SpriteBatch &dest,
        const BarTileset &tileset,
        const ivec2 &pos,
        const uvec2 &size,
        f32 fill,
        const vec4 &color_fill = vec4(0.0),
        const vec4 &color_bg = vec4(0.0)) const;
};
