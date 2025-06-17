#include "model/model_shrine.hpp"
#include "gfx/sprite_resource.hpp"
#include "gfx/render_context.hpp"
#include "entity/entity_render.hpp"
#include "entity/entity_shrine.hpp"

static const auto spritesheet =
    SpritesheetResource("entity_shrine", "entity/shrine.png", { 16, 16 });

ModelShrine::ModelShrine()
    : ModelHumanoid(
        spritesheet.get(),
        ModelHumanoid::Textures::from_default_spritesheet(
            *spritesheet)) {}

void ModelShrine::render(
    RenderContext &ctx,
    const RenderParams &params) const {
    const auto &entity = *params.entity;
    const auto passes =
        entity_render::adjust_passes(
            RENDER_FLAG_PASS_ALL_3D & ~RENDER_FLAG_PASS_TRANSPARENT,
            entity);

    auto &group = ctx.push_group();
    {
        group.program = &Renderer::get().programs["shrine"];
        group.group_bgfx_flags = BGFX_FLAGS_DEFAULT;
        group.group_passes = passes;
        entity_render::configure_group(group, entity);

        group.uniforms.set(
            "u_shrine_activated_ticks",
            vec4(params.entity->activated_ticks));
        group.uniforms.set(
            "u_shrine_color",
            vec4(params.entity->color, 1.0f));

        Base::render(
            ctx,
            {
                .entity = params.entity,
                .anim = params.anim
            });
    }
    ctx.pop_group();
}
