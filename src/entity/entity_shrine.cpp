#include "entity/entity_shrine.hpp"
#include "entity/entity_portal_particle.hpp"
#include "model/model_shrine.hpp"
#include "gfx/renderer_resource.hpp"
#include "util/hash.hpp"
#include "util/rand.hpp"
#include "serialize/serialize.hpp"

SERIALIZE_ENABLE()
DECL_SERIALIZER(EntityShrine)

static auto model_manager = RendererResource<ModelShrine>(
    []() { return std::make_unique<ModelShrine>(); });

void EntityShrine::render(RenderContext &ctx) {
    Base::render(ctx);

    ModelShrine::AnimationState animation_state;
    animation_state.swing = 0.5f;
    animation_state.rot_head = {{ 0, this->facing.xz_angle_lerp16() }};
    model_manager->render(
        ctx,
        {
            .entity = this,
            .anim = &animation_state,
        });
}

LightArray EntityShrine::lights() const {
    LightArray ls;
    if (global.time->ticks >= this->activated_ticks) {
        const auto t =
            math::clamp(
                (global.time->ticks - this->activated_ticks)
                    / static_cast<f32>(60),
                0.0f,
                1.0f);
        const auto pulse = math::time_wave(2.3f, this->id) * 0.1f;
        auto l = Light(
            this->center,
            (t + pulse) * 2.0f * this->color,
            2,
            false);
        l.att_quadratic = 4.0f;
        ls.try_add(l);
    }
    return ls;
}

const ModelShrine &EntityShrine::model() const {
    return model_manager.get();
}

void EntityShrine::tick() {
    Base::tick();
    const auto &keyboard = global.platform->get_input<Keyboard>();
    if (keyboard["p"]->is_pressed_tick()) {
        this->activated_ticks = global.time->ticks;
    }

    if (this->activated()
            && Rand(hash(this->id, global.time->ticks)).chance(0.01f)) {
        EntityParticle::spawn<EntityPortalParticle>(
            *this->level,
            this->center,
            this->color);
    }
}
