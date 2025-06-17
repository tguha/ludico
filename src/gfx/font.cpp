#include "gfx/font.hpp"
#include "gfx/sprite_resource.hpp"
#include "util/collections.hpp"
#include "util/string.hpp"

static const auto spritesheet =
    SpritesheetResource("font", "misc/font_alt.png", { 8, 8 });

void Font::init() {
    const auto &sprites = spritesheet.get();

    ASSERT(sprites.unit_size == GLYPH_SIZE);
    for (isize y = 0; y < sprites.size_units.y; y++) {
        for (isize x = 0; x < sprites.size_units.x; x++) {
            const auto i = y * sprites.size_units.x + x;
            this->glyphs[static_cast<char>(i)] =
                sprites[{ x, sprites.size_units.y - y - 1 }];
        }
    }
}

void Font::render(
    SpriteBatch &dst,
    const ivec2 &pos,
    char c,
    const vec4 &color,
    u64 flags,
    isize z) const {
    if (!this->glyphs.contains(c)) {
        c = '?';
    }

    const auto &glyph = this->glyphs.at(std::toupper(c));

    if (flags & DOUBLED) {
        dst.push(
            glyph,
            pos + ivec2(1, -1),
            vec4(
                math::clamp(color.rgb() - vec3(0.5f), vec3(0.0f), vec3(1.0f)),
                1.0f),
            color.a,
            z - 1);
    }

    dst.push(glyph, pos, vec4(color.rgb(), 1.0f), color.a, z);
}

void Font::render(
    SpriteBatch &dst,
    const ivec2 &pos,
    const std::string &str,
    const vec4 &color,
    u64 flags,
    isize z) const {
    for (usize i = 0; i < str.length(); i++) {
       this->render(
           dst,
           pos + ivec2(i * GLYPH_SIZE.x, 0),
           str[i],
           color,
           flags,
           z);
    }
}

List<Font::PosString> Font::fit(
    Allocator *allocator,
    const std::string &str,
    const AABB2u &box,
    usize alignment,
    usize spacing) {
    List<Font::PosString> dst(allocator);

    const auto max_width = box.size().x;

    // number of lines created
    usize num_lines = 0;

    // height remaining for use
    usize remaining_height = box.size().y;

    if (remaining_height < this->height()) {
        WARN("Box {} not large enough for any string (fitting {})", box, str);
        return dst;
    }

    // current line of strings
    usize current_line_width = 0;
    std::vector<std::string> current_line;

    // remaining words
    const std::string delim = " \t\n_\\/()[]{},.<>";
    auto words =
        collections::move_to<std::deque<std::string>>(
            strings::split_ndelim(
                str,
                delim,
                (alignment & ALIGN_RIGHT) ?
                    strings::ATTACH_RIGHT : strings::ATTACH_LEFT));

    // make a line out of current words[]
    auto make_line = [&]() {
        if (!current_line.empty()) {
            // remove width from delimiter on last (if align left or center) or
            // first (if align right) word
            const auto &w =
                current_line[
                    (alignment & ALIGN_RIGHT) ? 0 : (current_line.size() - 1)];

            isize i;
            std::string extra_delims;

            if (alignment & ALIGN_RIGHT) {
                i = w.length() - 1;
                while (i >= 0
                        && delim.find(w[i]) != std::string::npos) {
                    extra_delims += w[i];
                    i--;
                }
            } else {
                i = 0;
                while (i < isize(w.length())
                        && delim.find(w[i]) != std::string::npos) {
                    extra_delims += w[i];
                    i++;
                }
            }

            current_line_width -= this->width(extra_delims);
        }


        // add and align all words in current line
        usize x;
        if (alignment & ALIGN_CENTER_H) {
            x = box.min.x + (max_width - current_line_width) / 2;
        } else if (alignment & ALIGN_RIGHT) {
            x = box.min.x + max_width - current_line_width;
        } else {
            // align left
            x = box.min.x;
        }

        // remove elements from current line
        for (auto &w : current_line) {
            // use y to track current line
            const auto pos = ivec2(x, num_lines);
            x += this->width(w);
            dst.emplace(PosString { std::move(w), pos });
        }

        current_line.clear();
        num_lines++;
    };

    while (!words.empty()) {
        // check if we can add the next word into this line
        const auto word_width = this->width(words.front());

        if (word_width > max_width) {
            WARN(
                "Word {} in string {} is too large for box {}",
                words.front(), str, box);
            return dst;
        }

        if (current_line_width + word_width > max_width) {
            // make a line out of the current words
            make_line();

            // fail if not enough height to go to the next line
            if (remaining_height < (this->height() + spacing)) {
                WARN(
                    "Not enough space to fit string {} in box {}, {} is left",
                    str, box, words);
                return dst;
            }

            // go down, reset for new line
            remaining_height -= this->height() + spacing;
            current_line_width = 0;
        } else {
            // add word to line
            current_line_width += word_width;
            current_line.emplace_back(words.front());
            words.pop_front();
        }
    }

    // make the last line
    make_line();

    // align everything vertically
    const auto total_height =
        (num_lines * this->height()) + ((num_lines - 1) * spacing);

    isize y_top;
    if (alignment & ALIGN_CENTER_V) {
        y_top = box.max.y - ((box.size().y - total_height) / 2);
    } else if (alignment & ALIGN_BOTTOM) {
        y_top = box.min.y + total_height;
    } else {
        // align top
        y_top = box.max.y - this->height();
    }

    // recalculate heights
    // word line number is stored in pos.y
    for (auto &ps : dst) {
        ps.pos.y =
            y_top - ((ps.pos.y + 1) * this->height()) - (ps.pos.y * spacing);
    }

    return dst;
}
