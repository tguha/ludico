#include "ui/ui_renderer.hpp"
#include "gfx/sprite_resource.hpp"
#include "global.hpp"

static const auto
    spritesheet_base =
        SpritesheetResource("ui_base", "ui/base.png", { 16, 16 }),
    spritesheet_button =
        SpritesheetResource("ui_button", "ui/button.png", { 16, 16 });

void UIRenderer::Tilesets::init() {
    const auto
        &sprites_base = spritesheet_base.get(),
        &sprites_button = spritesheet_button.get();

    this->data[TS_BASE] = UITileset::from(sprites_base, { 0, 0 });
    this->data[TS_BUTTON_BASE] = UITileset::from(sprites_button, { 0, 0 });
    this->data[TS_BUTTON_PRESS] = UITileset::from(sprites_button, { 3, 0 });
}

void UIRenderer::init() {
    this->tilesets.init();
}

void UIRenderer::render_tile_fill(
    SpriteBatch &dest,
    const TextureArea &tile,
    const ivec2 &pos,
    const uvec2 &size,
    const vec4 &color) const {
    const auto tile_size = tile.pixels().size();
    ASSERT(size.x % tile_size.x == 0);
    ASSERT(size.y % tile_size.y == 0);

    ivec2 p(0);
    while (p.x < i32(size.x)) {
        while (p.y < i32(size.y)) {
            dest.push(tile, p, color);
            p.y += tile_size.y;
        }

        p.x += tile_size.x;
        p.y = 0;
    }
}

// TODO: some sizes need to be fixed, such as 34x34
void UIRenderer::render_background(
    SpriteBatch &dest,
    const UITileset &tileset,
    const ivec2 &pos,
    const uvec2 &size,
    const vec4 &color) const {
    const auto tile_size = tileset.tile_size();
    ASSERT(
        size.x >= tile_size.x
        && size.y >= tile_size.y);

    ivec2 p(0);
    while (p.x < i32(size.x)) {
        while (p.y < i32(size.y)) {
            const auto
                e_bottom = p.y == 0,
                e_top = p.y + tile_size.y >= size.y,
                e_left = p.x == 0,
                e_right = p.x + tile_size.x >= size.x;

            if (e_top) {
                if (e_left) {
                    dest.push(
                        tileset[UITileset::CORNER_TL],
                        pos + ivec2(0, size.y - tile_size.y),
                        color);
                }

                if (e_right) {
                    dest.push(
                        tileset[UITileset::CORNER_TR],
                        pos + ivec2(size - tile_size),
                        color);
                }

                if (!(e_left || e_right)) {
                    dest.push(
                        tileset[UITileset::EDGE_TOP],
                        pos + ivec2(
                            math::min<i32>(p.x, size.x - tile_size.x),
                            size.y - tile_size.y),
                        color);
                }
            }

            if (e_bottom) {
                if (e_left) {
                    dest.push(
                        tileset[UITileset::CORNER_BL],
                        pos + math::min(p, ivec2(size - tile_size)),
                        color);
                }

                if (e_right) {
                    dest.push(
                        tileset[UITileset::CORNER_BR],
                        pos + ivec2(size.x - tile_size.x, 0),
                        color);
                }

                if (!(e_left || e_right)) {
                    dest.push(
                        tileset[UITileset::EDGE_BOTTOM],
                        pos + math::min(p, ivec2(size - tile_size)),
                        color);
                }
            }

            if (!(e_top || e_bottom)) {
                if (e_left) {
                    dest.push(
                        tileset[UITileset::EDGE_LEFT],
                        pos + math::min(p, ivec2(size - tile_size)),
                        color);
                }

                if (e_right) {
                    dest.push(
                        tileset[UITileset::EDGE_RIGHT],
                        pos + ivec2(
                            size.x - tile_size.x,
                            math::min<i32>(p.y, size.y - tile_size.y)),
                        color);
                }
            }

            if (!e_top && !e_bottom && !e_left && !e_right) {
                dest.push(
                    tileset[UITileset::CENTER],
                    pos + p,
                    color);
            }

            p.y += tile_size.y;
        }

        p.x += tile_size.x;
        p.y = 0;
    }
}

void UIRenderer::render_bar(
    SpriteBatch &dest,
    const BarTileset &tileset,
    const ivec2 &pos,
    const uvec2 &size,
    f32 fill,
    const vec4 &color_fill,
    const vec4 &color_bg) const {
    constexpr auto SPRITE_SIZE = uvec2(8, 8);

    const auto &sprites = TextureAtlas::get()["ui_stats"];
    const std::array<TextureArea, 3> sprites_bg =
        {
            sprites[{ 0, 0 }],
            sprites[{ 1, 0 }],
            sprites[{ 2, 0 }],
        };
    ASSERT(size.y == SPRITE_SIZE.y);

    fill = math::clamp(fill, 0.0f, 1.0f);

    // draw background
    for (usize x = 0; x < size.x; x += SPRITE_SIZE.x) {
        auto offset = ivec2(x, 0);
        TextureArea sprite = sprites_bg[1];

        if (x == 0) {
            sprite = sprites_bg[0];
        } else if (x + 8 >= size.x) {
            sprite = sprites_bg[2];
            offset.x = size.x - 8;
        }

        dest.push(sprite, pos + offset, color_bg);
    }

    const auto limit = isize(math::floor(fill * size.x));
    isize x = limit;
    while (x > 0) {
        const auto r = limit - x;

        if (r < SPRITE_SIZE.x && fill >= 1.0f) {
            // draw filled end
            dest.push(
                tileset.right,
                pos + ivec2(limit - 8, 0),
                color_fill);
            x -= 8;
        } else if (r < 3) {
            // draw partial end
            dest.push(
                tileset.end,
                pos + ivec2(math::max<isize>(limit - 3, 0), 0),
                color_fill);
            x -= 3;
        } else if (r < SPRITE_SIZE.x) {
            // partial
            x--;
            dest.push(
                tileset.partial,
                pos + ivec2(math::max<isize>(x, 0), 0),
                color_fill);
        } else if (x <= SPRITE_SIZE.x) {
            // full left
            dest.push(
                tileset.left,
                pos + ivec2(0, 0),
                color_fill);
            x -= 8;
        } else {
            // center
            dest.push(
                tileset.center,
                pos + ivec2(x - 8, 0),
                color_fill);
            x -= 8;
        }
    }
}
