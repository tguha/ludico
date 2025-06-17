#include "entity/entity_sparkle_particle.hpp"
#include "gfx/game_camera.hpp"
#include "gfx/renderer.hpp"
#include "gfx/particle_renderer.hpp"
#include "serialize/serialize.hpp"

SERIALIZE_ENABLE()
DECL_SERIALIZER(EntitySparkleParticle)

void EntitySparkleParticle::tick() {
    Base::tick();

    const auto p =
        f32(global.time->ticks - this->spawned_tick)
            / f32(this->ticks_to_live) ;
    const auto t = math::sin(math::PI * p);
    this->color = vec4(t * this->base_color.rgb(), 1.0f);
}

EntitySparkleParticle::Ref EntitySparkleParticle::spawn_on_entity(
    const Entity &e,
    const vec3 &color) {
    // get screen-space AABB for entity
    const auto aabb = OPT_OR_RET(e.aabb(), Ref::none());
    const auto aabb_px =
        OPT_OR_RET(
            global.game->camera->to_screen_bounds(aabb),
            Ref::none());

    // find random spot on screen-space AABB which is entity
    constexpr auto ATTEMPTS = 4;
    auto rand = Rand(hash(e.id, e.pos, global.time->ticks));

    ivec2 pos;
    usize n = 0;
    while (n < ATTEMPTS) {
        pos = rand.next(aabb_px.min, aabb_px.max);
        if (global.game->camera->entity_at(pos).id == e.id) {
            break;
        }
        n++;
    }


    if (n == ATTEMPTS) {
        return Ref::none();
    }

    // cast ray into entity AABB at screen point
    const auto ray = global.game->camera->ray_from(pos);
    const auto hit = OPT_OR_RET(ray.intersect_aabb(aabb), Ref::none());

    // spawn particle at specified hit location and 1 voxel towards camera
    return e.level->spawn<EntitySparkleParticle>(
        hit + (-ray.direction * (1.0f / SCALE)), color);
}
