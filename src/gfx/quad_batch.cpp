#include "gfx/quad_batch.hpp"
#include "gfx/sprite_renderer.hpp"
#include "gfx/renderer.hpp"
#include "global.hpp"

void QuadBatch::render(RenderState render_state) const {
    if (this->quads.empty()) {
        return;
    }

    Renderer::get().sprite_renderer->render(*this, render_state);
}
