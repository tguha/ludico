#include "ui/ui_item_description.hpp"
#include "ui/cursor_attachment_item.hpp"
#include "ui/ui_item_slot.hpp"
#include "ui/ui_root.hpp"
#include "gfx/font.hpp"
#include "gfx/renderer.hpp"
#include "global.hpp"
#include "util/color.hpp"

void UIItemDescription::reposition() {
    this->align(UIObject::ALIGN_RIGHT | UIObject::ALIGN_BOTTOM);
    this->pos += SPACING;
}

void UIItemDescription::render(SpriteBatch &batch, RenderState render_state) {
    if (this->fade.done()) {
        return;
    }

    StackAllocator<4096> allocator;
    auto &renderer = Renderer::get();

    const auto [count, name, hold_ticks, lines] = this->fade.last();

    const auto fs_name =
        renderer.font->fit(
            &allocator,
            name,
            BOX_NAME.translate(uvec2(this->pos_absolute())),
            Font::ALIGN_RIGHT | Font::ALIGN_BOTTOM,
            1);

    // name is multiline? move count up
    const auto is_name_multiline =
        std::any_of(
            fs_name.begin(),
            fs_name.end(),
            [&](const auto &ps) {
                return ps.pos.y != fs_name[0].pos.y;
            });

    const auto fs_count =
        renderer.font->fit(
            &allocator,
            count == 1 ? "" : fmt::format("{}x", count),
            BOX_COUNT
                .translate(uvec2(this->pos_absolute()))
                .translate({
                    SIZE.x - BOX_COUNT.size().x,
                    BOX_NAME.size().y
                        + (is_name_multiline ?
                            renderer.font->height()
                            : 0)
                }),
            Font::ALIGN_RIGHT | Font::ALIGN_BOTTOM,
            0);

    const auto a = this->fade.value();
    renderer.font->render(
        batch, fs_name, vec4(vec3(1.0f), a), Font::DOUBLED);
    renderer.font->render(
        batch, fs_count,
        vec4(color::argb32_to_v(0xFFE6D55A).rgb(), a),
        Font::DOUBLED);

    // render any extra lines
    const auto y_base =
        (fs_count[0].str.empty() ?
            fs_name[fs_name.size() - 1].pos.y
            : fs_count[0].pos.y)
            + renderer.font->height();
    usize y = 0;
    for (usize i = 0; i < lines.size(); i++) {
        const auto fs =
            renderer.font->fit(
                &allocator,
                lines[i].text,
                AABB2u({ BOX_LINES.size().x, BOX_LINES.size().y - y })
                    .translate(uvec2(this->pos_absolute()))
                    .translate({ 0, y_base + y }),
                Font::ALIGN_BOTTOM | Font::ALIGN_RIGHT,
                0);

        renderer.font->render(
            batch,
            fs,
            vec4(lines[i].color.rgb(), a),
            Font::DOUBLED);

        // TODO: will break if a line gets broken up into multiple lines
        y += renderer.font->height();
    }
}
