#include "gfx/debug_renderer.hpp"
#include "gfx/renderer.hpp"
#include "gfx/util.hpp"
#include "constants.hpp"

DebugRenderer::DebugRenderer(const Renderer &renderer)
    : renderer(renderer) {
    this->vertex_buffer =
        gfx::as_bgfx_resource(
            bgfx::createDynamicVertexBuffer(
                1024,
                VertexColorNormal::layout,
                BGFX_BUFFER_ALLOW_RESIZE));
    this->index_buffer =
        gfx::as_bgfx_resource(
            bgfx::createDynamicIndexBuffer(
                1024,
                BGFX_BUFFER_ALLOW_RESIZE));
}

void DebugRenderer::render(RenderState render_state) const {
    if (!(render_state.flags &
          (RENDER_FLAG_PASS_BASE | RENDER_FLAG_PASS_TRANSPARENT))) {
        return;
    }

    render_state = render_state.or_defaults();

    const auto &buffer =
        this->buffers[
            (render_state.flags & RENDER_FLAG_PASS_TRANSPARENT) ?
                PASS_TRANSPARENT
                : PASS_DEFAULT];

    if (buffer.num_indices() > 0 && buffer.num_indices() > 0) {
        this->renderer.render_buffers(
            this->renderer.programs.at("debug"),
            mat4(1.0),
            VertexColorNormal::layout,
            std::span { buffer.vertices },
            std::span { buffer.indices },
            render_state);
    }
}

void DebugRenderer::reset() {
    for (auto &b : this->buffers) {
        b.reset();
    }
}
