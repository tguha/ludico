#include "gfx/sprite_batch.hpp"
#include "gfx/sprite_renderer.hpp"
#include "gfx/renderer.hpp"
#include "global.hpp"

void SpriteBatch::render(RenderState render_state) const {
    if (this->sprites.empty()) {
        return;
    }

    // sort sprites so that low z-index sprites are drawn first
    std::sort(
        this->sprites.begin(),
        this->sprites.end(),
        [](const auto &a, const auto &b) {
            return a.z == b.z ? (a.i < b.i) : (a.z < b.z);
        });

    Renderer::get().sprite_renderer->render(*this, render_state);
}
