#include "entity/entity_particle.hpp"
#include "gfx/particle_renderer.hpp"
#include "gfx/render_context.hpp"
#include "gfx/renderer.hpp"
#include "serialize/serialize.hpp"

SERIALIZE_ENABLE()
DECL_SERIALIZER(EntityParticle)

void EntityParticle::tick() {
    Base::tick();

    if (this->spawned_tick + this->ticks_to_live <= global.time->ticks) {
        this->destroy();
    }
}

void EntityParticle::render(
    RenderContext &ctx) {
    ctx.push(
        RenderCtxFn
        {
            [this](const RenderGroup&, RenderState render_state) {
                Renderer::get().particle_renderer->enqueue(
                    { this->pos, this->color });
            },
            RENDER_FLAG_PASS_ALL_3D & ~RENDER_FLAG_PASS_TRANSPARENT
        });
}
