#include "entity/entity_render.hpp"
#include "gfx/render_context.hpp"

// returns true if this entity is the blocking target
static bool is_target_for_blocking(const Entity &entity) {
    return EntityRef(entity) == global.game->camera_entity;
}

u8 entity_render::adjust_passes(
    u8 passes,
    const Entity &entity) {
    if (is_target_for_blocking(entity)) {
        passes |= RENDER_FLAG_PASS_ENTITY_STENCIL;
    } else {
        passes &= ~RENDER_FLAG_PASS_ENTITY_STENCIL;
    }
    return passes;
}

u8 entity_render::adjust_flags(
    u8 flags,
    const Entity &entity) {
    // stencil for use with occlusion calculations if this entity is a blocking
    // target
    if (is_target_for_blocking(entity)) {
        flags |= GFX_FLAG_ENTITY_STENCIL;
    }
    return flags;
}

void entity_render::configure_group(
    RenderGroup &group,
    const Entity &entity) {
    group.group_render_flags |= RENDER_FLAG_CVP;

    global.game->camera->compute_rounded_camera(entity.pos)
        .uniforms(
            group.uniforms,
            "u_cvp");

    group.group_gfx_flags =
        entity_render::adjust_flags(group.group_gfx_flags, entity);

    group.group_passes =
        entity_render::adjust_passes(group.group_passes, entity);

    group.group_entity_id = entity.id;
}

mat4 entity_render::model_base(const Entity &entity) {
    auto m = mat4(1.0);
    m = math::translate(m, entity.pos);
    m = math::scale(m, 1.0f / vec3(SCALE));
    return m;
}

mat4 entity_render::model_billboard(const Entity &entity) {
    auto m = model_base(entity);
    m *= math::dir_to_rotation(
            math::xz_to_xyz(global.game->camera->rotation_direction(), 0.0f));
    return m;
}
