#include "state/state_game.hpp"
#include "entity/entity.hpp"
#include "entity/entity_daisy.hpp"
#include "entity/entity_player.hpp"
#include "entity/entity_dandelion.hpp"
#include "entity/entity_portal.hpp"
#include "entity/entity_shrine.hpp"
#include "entity/entity_sunflower.hpp"
#include "entity/entity_hydrangea.hpp"
#include "gfx/entity_highlighter.hpp"
#include "gfx/font.hpp"
#include "gfx/game_camera.hpp"
#include "gfx/particle_renderer.hpp"
#include "gfx/renderer.hpp"
#include "gfx/sprite_batch.hpp"
#include "gfx/sun.hpp"
#include "gfx/debug_renderer.hpp"
#include "gfx/vignette_renderer.hpp"
#include "input/input.hpp"
#include "levelgen/gen.hpp"
#include "level/level.hpp"
#include "level/level_renderer.hpp"
#include "level/chunk.hpp"
#include "platform/platform.hpp"
#include "ui/ui_root.hpp"
#include "ui/ui_hud.hpp"
#include "util/color.hpp"
#include "util/ray.hpp"
#include "util/time.hpp"
#include "util/file.hpp"
#include "serialize/serialize.hpp"
#include "low_power_renderer.hpp"
#include "occlusion_map.hpp"
#include "resources.hpp"
#include "constants.hpp"
#include "global.hpp"

// TODO: remove
#include "serialize/context_level.hpp"

void StateGame::init() {
    this->allocator =
        BumpAllocator(
            &global.allocator,
            64 * 1024 * 1024);

    this->level =
        make_alloc_ptr<Level>(
            this->allocator,
            &this->allocator,
            0,
            ivec2(2, 2),
            [](Level &level) {
                DefaultLevelGenerator().generate(level);
            });

    this->level_renderer =
        make_alloc_ptr<LevelRenderer>(this->allocator, *this->level);

    this->camera =
        make_alloc_ptr<GameCamera>(this->allocator, *this->level);

    this->occlusion_map =
        make_alloc_ptr<OcclusionMap>(this->allocator);
    this->occlusion_map->init();

    this->sun =
        make_alloc_ptr<Sun>(this->allocator);

    const auto aabb_l = this->level->aabb();
    const auto center_l = ivec2(aabb_l.center().xz());
    this->player = this->level->spawn_at_xz<EntityPlayer>(center_l);
    this->camera_entity = this->player;
    ASSERT(this->player);

    /* auto rand = Rand(0x12345); */
    /* for (isize x = -32; x <= 32; x++) { */
    /*     for (isize z = -32; z <= 32; z++) { */
    /*         const auto pos = */
    /*             math::xz_to_xyz(center_l, 0) */
    /*                 + ivec3(x, 6, z); */

    /*         if (this->level->tiles[pos] != 0 */
    /*             || rand.chance(0.70f)) { */
    /*             continue; */
    /*         } */

    /*         EntityPlant::Ref entity; */
    /*         switch (rand.next(0, 3)) { */
    /*             case 0: */
    /*                 entity = */
    /*                     this->level->spawn_at_xz<EntityHydrangea>(pos.xz()); */
    /*                 break; */
    /*             case 1: */
    /*                 entity = */
    /*                     this->level->spawn_at_xz<EntityDandelion>(pos.xz()); */
    /*                 break; */
    /*             case 2: */
    /*                 entity = */
    /*                     this->level->spawn_at_xz<EntityDaisy>(pos.xz()); */
    /*                 break; */
    /*             case 3: */
    /*                 entity = */
    /*                     this->level->spawn_at_xz<EntitySunflower>(pos.xz()); */
    /*                 break; */
    /*         } */

    /*         if (entity) { */
    /*             entity->stage = rand.next(0, entity->num_stages() - 1); */
    /*         } */
    /*     } */
    /* } */

    this->hud = &global.ui->add(std::make_unique<UIHUD>());
    this->hud->set_player(*this->player);
    this->hud->configure();
}

void StateGame::destroy() {

}

void StateGame::tick() {
    const auto &keyboard = global.platform->get_input<Keyboard>();

    if (keyboard["k"]->is_pressed_tick()) {
        SerializationContextLevel ctx;
        ctx.base_allocator = &global.allocator;
        ctx.level = this->level.get();

        ElasticArchive archive(64);
        Serializer<alloc_ptr<Level>>()
            .serialize(this->level, archive, ctx);

        file::write_file("save.dat", archive.buffer());
    } else if (keyboard["l"]->is_pressed_tick()) {
        SerializationContextLevel ctx;
        ctx.base_allocator = &global.allocator;
        ctx.level = nullptr;

        auto data = file::read_file("save.dat").unwrap();
        Archive archive(std::span { data });
        Serializer<alloc_ptr<Level>>()
            .deserialize(this->level, archive, ctx);
        ctx.resolve();

        this->player = ctx.player;
        this->camera->level = this->level.get();
        this->camera_entity = this->player;
        this->level_renderer =
            make_alloc_ptr<LevelRenderer>(
                this->allocator,
                *this->level);
        this->hud->set_player(*this->player);
    }

    if (Entity *entity = this->camera_entity) {
        this->camera->follow(*entity);
    }

    Renderer::get().entity_highlighter->tick();

    this->camera->tick(*this->level);
    this->level->tick();

    // TODO: move view controls elsewhere
    if (keyboard["["]->is_pressed_tick()) { --camera->zoom; }
    if (keyboard["]"]->is_pressed_tick()) { ++camera->zoom; }

    if (keyboard[","]->is_pressed_tick()) {
        camera->rotation =
            static_cast<GameCamera::Rotation>(
                ((camera->rotation - 1) + GameCamera::ROT_COUNT)
                    % GameCamera::ROT_COUNT);
    }

    if (keyboard["."]->is_pressed_tick()) {
        camera->rotation =
            static_cast<GameCamera::Rotation>(
                ((camera->rotation + 1) + GameCamera::ROT_COUNT)
                    % GameCamera::ROT_COUNT);
    }

    // TODO: move elsewhere
    if (!global.ui->has_mouse()) {
        if (const auto e = this->camera->mouse_entity()) {
            if (e->highlight()) {
                const auto color =
                    vec4(color::argb32_to_v(0xffffa53c).rgb(), 0.75f);
                const auto a = 0.2f * math::time_wave(1.0f);
                Renderer::get().entity_highlighter->set({
                        e->id,
                        vec4(color.rgb(), math::clamp(color.a + a, 0.0f, 1.0f))
                    });
            }
        }
    }

    this->occlusion_map->update(
        *this->camera,
        *this->level,
        Level::to_tile(this->camera->pos.xz()));
}

void StateGame::update() {
    this->level->update();
}

void StateGame::render() {
    auto &renderer = Renderer::get();

    // clear to black
    renderer.clear_color = color::rgba32_to_v(0xFFd5de08);

    this->sun->ambient = vec3(0.6f);
    this->sun->diffuse = vec3(0.45f);
    this->sun->direction = math::normalize(vec3(0.70f, -0.80f, -0.452f));
    this->sun->update(
        renderer.textures["sun_depth"], this->camera->camera);

    this->camera->update();

    // retreive lights for compositing
    // TODO: prioritize with more than MAX_LIGHTS lights
    auto lights = global.frame_allocator.alloc_span<Light>(1024);
    const auto [n_lights, _] = level->lights(
        lights,
        this->camera->light_bounds().ntransform<3>(
            [](ivec2 v) { return math::xz_to_xyz(v, 0); }));

    renderer.composite(
        *this->camera,
        [&](RenderState render_state) {
            this->render_3d(render_state);
        },
        [&](RenderState render_state) { this->render_ui(render_state); },
        *this->sun,
        *this->occlusion_map,
        lights.subspan(0, math::min<usize>(n_lights, MAX_LIGHTS)));

    renderer.debug->reset();
}

void StateGame::render_3d(RenderState render_state) {
    auto &renderer = Renderer::get();

    if (render_state.flags & RENDER_FLAG_PASS_FIRST) {
        this->level_renderer->render();
    }

    this->level_renderer->pass(render_state);
    renderer.particle_renderer->flush(render_state);
    renderer.debug->render(render_state);
}

void StateGame::render_ui(RenderState render_state) {
    SpriteBatch batch_s(&global.frame_allocator);
    QuadBatch batch_q(&global.frame_allocator);

    // TODO: generify to screen effect list
    LowPowerRenderer::instance()
        .render(*this->player, batch_s, render_state);

    global.ui->render(batch_s, render_state);

    if (global.debug.ui_debug.enabled) {
        global.ui->render_debug(batch_s, batch_q, render_state);
    }

    batch_s.render(render_state);
    batch_q.render(render_state);
}
