#include "gfx/icon_renderer.hpp"
#include "gfx/renderer.hpp"
#include "gfx/texture_reader.hpp"
#include "global.hpp"

static constexpr auto LABEL_PREFIX = "ICON_";

void IconRenderer::add(
    const std::string &label,
    const uvec2 &size,
    InplaceRenderFn &&render_fn) {
    this->unrendered.push(
        std::make_unique<PendingIcon>(std::move(render_fn), label, size));
}

std::optional<const TextureAtlasEntry*> IconRenderer::get(
    const std::string &label) const {
    const auto l = LABEL_PREFIX + label;
    return TextureAtlas::get().contains(l) ?
        std::make_optional(&TextureAtlas::get()[LABEL_PREFIX + label])
        : std::nullopt;
}

void IconRenderer::update() {
    auto &ex_reader = Renderer::get().ex_reader;

    // check if data is ready
    if (ex_reader->ready_frame == Renderer::get().frame_number) {
        while (!this->pending.empty()) {
            auto r = std::move(this->pending.front());
            this->pending.pop();

            auto pixels =
                global.frame_allocator.alloc_span<u8>(
                    r->size.x * r->size.y * 4);

            auto px = Pixels(&pixels[0], r->size);
            for (usize y = 0; y < r->size.y; y++) {
                for (usize x = 0; x < r->size.x; x++) {
                    px.rgba({ x, y }) = ex_reader->get<u32>({ x, y });
                }
            }

            const auto &entry =
                TextureAtlas::get().add(
                    LABEL_PREFIX + r->label,
                    &pixels[0],
                    r->size, r->size);
            LOG(
                "Added new icon {} with size {} (offset {})",
                r->label, r->size, entry.offset);
        }
    }

    // can only render on frames where request has not been made, meaning that
    // request will be made this frame
    if (!ex_reader->requested && !this->unrendered.empty()) {
        auto r = std::move(this->unrendered.front());
        this->unrendered.pop();

        // pre-configure view rect to only cover icon size
        bgfx::setViewRect(
            Renderer::VIEW_EX,
            0, ex_reader->texture->size.y - r->size.y,
            r->size.x, r->size.y);

        r->render_fn(
            RenderState(Renderer::VIEW_EX, 0, 0));

        // move to pending
        this->pending.push(std::move(r));
    }
}
