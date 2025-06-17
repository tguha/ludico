#include "model/model_player.hpp"
#include "entity/entity_render.hpp"
#include "entity/entity_player.hpp"
#include "gfx/sprite_resource.hpp"
#include "gfx/render_context.hpp"
#include "item/item_icon.hpp"
#include "item/item.hpp"

static const auto spritesheet =
    SpritesheetResource("entity_player", "entity/player.png", { 16, 16 });

ModelPlayer::ModelPlayer()
    : ModelHumanoid(
        spritesheet.get(),
        ModelHumanoid::Textures::from_default_spritesheet(
            *spritesheet)) {
    const auto &sprites = spritesheet.get();

    LOG("{}", spritesheet[{ 0, 0 }].pixels());
    this->blinker =
        &this->mesh.add(
            VoxelModel::from_sprite(
                sprites,
                sprites[{ 4, 0 }],
                SpecularTextures::instance().min(),
                1,
                VoxelModel::FLAG_MIN_BOUNDS));
}

mat4 ModelPlayer::model_blinker(
    const EntityPlayer &entity,
    const AnimationState &anim,
    const mat4 &base) const {
    auto m = this->model_head(entity, anim, base);
    const auto
        size_head = this->head->size(),
        size_blinker = this->blinker->size();
    m = math::translate(
        m,
        math::xyz_to_xnz((size_head - size_blinker) / 2.0f, size_head.y));
    m = math::translate(m, size_blinker / 2.0f);
    m = math::rotate(m, math::PI_2, vec3(1, 0, 0));
    m = math::translate(m, -size_blinker / 2.0f);
    return m;
}

void ModelPlayer::render(
    RenderContext &ctx,
    const RenderParams &params) const {
    const auto &entity = *params.entity;

    const auto passes =
        entity_render::adjust_passes(
            RENDER_FLAG_PASS_ALL_3D & ~RENDER_FLAG_PASS_TRANSPARENT,
            entity);

    auto &group = ctx.push_group();
    {
        group.program = &Renderer::get().programs["model"];
        group.group_bgfx_flags = BGFX_FLAGS_DEFAULT;
        group.group_passes = passes;
        entity_render::configure_group(group, entity);

        Base::render(
            ctx,
            {
                .entity = params.entity,
                .anim = params.anim,
            });

        // render tool in off-hand
        if (entity.held_tool) {
            entity.held_tool->item().icon().render(
                ctx,
                &*entity.held_tool,
                this->model_hold(
                    entity,
                    *params.anim,
                    this->model_base(entity, *params.anim),
                    entity.side_tool(),
                    entity.held_tool));
        }

        // render blinker model
        const auto base_model =
            this->model_base(*params.entity, *params.anim);
        ctx.push(
            ModelPart {
                .entry = this->blinker,
                .model =
                    this->model_blinker(
                        *params.entity, *params.anim, base_model),
                .passes = passes
            });
    }
    ctx.pop_group();
}
