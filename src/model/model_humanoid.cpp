#include "model/model_humanoid.hpp"
#include "entity/entity_mob.hpp"
#include "entity/entity_render.hpp"
#include "item/item_icon.hpp"
#include "item/item.hpp"
#include "gfx/renderer.hpp"
#include "gfx/render_context.hpp"
#include "constants.hpp"
#include "global.hpp"

ModelHumanoid::ModelHumanoid(
    const MultiTexture &texture,
    const Textures &textures)
    : mesh(&Renderer::get().allocator) {
    this->head =
        &this->mesh.add(
            VoxelModel::from_sprites(
                texture,
                textures.head.tex_opt(),
                textures.head.spec_opt(),
                {},
                VoxelModel::FLAG_MIN_BOUNDS,
                VoxelShape::reshape_remove_corners));

    this->body =
        &this->mesh.add(
            VoxelModel::from_sprites(
                texture,
                textures.body.tex_opt(),
                textures.body.spec_opt(),
                {},
                VoxelModel::FLAG_MIN_BOUNDS,
                VoxelShape::reshape_remove_corners));

    this->arm =
        &this->mesh.add(
            VoxelModel::from_sprites(
                texture,
                textures.arm.tex_opt(),
                textures.arm.spec_opt(),
                {},
                VoxelModel::FLAG_MIN_BOUNDS,
                VoxelShape::reshape_remove_corners));

    this->leg =
        &this->mesh.add(
            VoxelModel::from_sprites(
                texture,
                textures.leg.tex_opt(),
                textures.leg.spec_opt(),
                {},
                VoxelModel::FLAG_MIN_BOUNDS,
                VoxelShape::reshape_remove_corners));

    this->head_offset_px =
        ivec3(
            0,
            this->leg->size().y + this->body->size().y,
            0);

    this->body_offset_px =
        ivec3(
            (this->head->size().x - this->body->size().x) / 2,
            this->leg->size().y,
            (this->head->size().z - this->body->size().z) / 2);
}

AABB ModelHumanoid::aabb() const {
    return AABB::merge(
        this->body->bounds()
            .translate(vec3(this->body_offset_px)),
        this->head->bounds()
            .translate(vec3(this->head_offset_px)),
        this->leg->bounds())
            .scale_min(1.0f / SCALE);
}

AABB ModelHumanoid::entity_aabb(const Entity &entity) const {
    return this->aabb();
}

f32 ModelHumanoid::rot_swing_arm(
    const EntityMob &entity,
    const AnimationState &anim,
    Side::Enum side) const {
    const auto swing_period_offset =
        math::PI_8 + (side == Side::LEFT ? 0 : math::PI);

    f32 rot_swing = 0.0f;
    if (!anim.rot_arms[side].done()) {
        rot_swing = anim.rot_arms[side].get();
    } else if (anim.swing) {
        rot_swing =
            (math::PI_16 / 1.5f)
                * math::sin(
                    (math::TAU * (*anim.swing))
                        + swing_period_offset);
    }

    return rot_swing;
}

f32 ModelHumanoid::rot_swing_leg(
    const EntityMob &entity,
    const AnimationState &anim,
    Side::Enum side) const {
    const auto swing_period_offset =
        side == Side::LEFT ? 0 : math::PI;

    f32 rot_swing = 0.0f;
    if (!anim.rot_legs[side].done()) {
        rot_swing = anim.rot_legs[side].get();
    } else if (anim.swing) {
        rot_swing =
            math::PI_16 +
                (math::PI_8
                    * math::sin(
                        (math::TAU * (*anim.swing))
                            + swing_period_offset));
    }

    return rot_swing;
}

mat4 ModelHumanoid::model_base(
    const EntityMob &entity,
    const AnimationState &anim) const {
    const auto size_body = this->body->size();
    const auto center_offset =
        (vec3(size_body.x, 0.0f, size_body.z)
            + vec3(
                this->body_offset_px.x,
                0.0f,
                this->body_offset_px.z)) / 2.0f;

    auto m = mat4(1.0f);
    m = math::translate(m, entity.pos);
    m = math::scale(m, vec3(1.0f / SCALE));
    m = math::translate(m, +center_offset);
    m = math::rotate(
        m, entity.direction.xz_angle_lerp16(), vec3(0, 1, 0));
    m = math::translate(m, -center_offset);
    return m;
}

mat4 ModelHumanoid::model_lower(
    const EntityMob &entity,
    const AnimationState &anim,
    const mat4 &base) const {
    const auto
        size_body = this->body->size(),
        size_leg = this->leg->size();
    const auto tilt_offset =
        vec3(0.0f, size_body.y + size_leg.y, 0.0f);
    auto m = base;
    m = math::translate(m, +tilt_offset);
    m = math::rotate(
        m, anim.tilt_body.get(), vec3(1, 0, 0));
    m = math::translate(m, -tilt_offset);
    return m;
}

mat4 ModelHumanoid::model_head(
    const EntityMob &entity,
    const AnimationState &anim,
    const mat4 &base) const {
    const auto
        size_head = this->head->bounds().size();
    auto m = base;
    m = math::translate(m, vec3(this->head_offset_px));
    m = math::translate(m, size_head / 2.0f);
    m = math::rotate(
        m,
        anim.rot_head.get()
            - entity.direction.xz_angle_lerp16(),
        vec3(0, 1, 0));
    m = math::translate(m, -size_head / 2.0f);
    return m;
}

mat4 ModelHumanoid::model_body(const EntityMob &entity,
    const AnimationState &anim, const mat4 &base) const {
    return math::translate(base, vec3(this->body_offset_px));
}

// arm model matrix
mat4 ModelHumanoid::model_arm(
    const EntityMob &entity,
    const AnimationState &anim,
    const mat4 &base,
    Side::Enum side) const {
    const auto
        size_arm = this->arm->size(),
        size_body = this->body->size();

    const auto
        swing_offset =
             vec3(
                size_arm.x / 2.0f,
                size_arm.y,
                size_arm.z / 2.0f),
        rot_offset = size_arm / vec3(2, 1, 2);

    auto m = base;
    m = math::translate(m, vec3(this->body_offset_px));
    m = math::translate(
        m,
        side == Side::LEFT ?
            vec3(-size_arm.x + 2.5f, 1, 1 + 0.01f)
            : vec3(size_body.x - 2.5f, 1, 1 + 0.01f));
    m = math::translate(m, rot_offset);
    m = math::rotate(
        m,
        side == Side::LEFT ? -math::PI_8 : math::PI_8,
        vec3(0, 0, 1));
    m = math::translate(m, -rot_offset);
    m = math::translate(m, swing_offset);
    m = math::rotate(m, this->rot_swing_arm(entity, anim, side), vec3(1, 0, 0));
    m = math::translate(m, -swing_offset);
    return m;
}

// leg model matrix
mat4 ModelHumanoid::model_leg(
    const EntityMob &entity,
    const AnimationState &anim,
    const mat4 &base,
    Side::Enum side) const {
    const auto
        size_leg = this->leg->size(),
        size_body = this->body->size();

    const auto swing_offset_leg =
         vec3(
            -size_leg.x / 2.0f,
            -size_leg.y,
            -size_leg.z / 2.0f);

    auto m = base;
    m = math::translate(m, vec3(this->body_offset_px));
    m = math::translate(
        m,
        side == Side::LEFT ?
            vec3(1, -size_leg.y, 1)
            : vec3(size_body.x - 1 - size_leg.x, - size_leg.y, 1));
    m = math::translate(m, -swing_offset_leg);
    m = math::rotate(
        m,
        this->rot_swing_leg(entity, anim, side),
        vec3(1, 0, 0));
    m = math::translate(m, swing_offset_leg);
    return m;
}

// model matrix for held item
mat4 ModelHumanoid::model_hold(
    const EntityMob &entity,
    const AnimationState &anim,
    const mat4 &base,
    Side::Enum side,
    ItemRef item) const {
    auto m = this->model_arm(entity, anim, base, side);

    const auto hold_type =
        item ? item->item().hold_type() : Item::HoldType::DEFAULT;

    auto m_trans =
        math::translate(
            mat4(1.0),
            item ?
                item->item().hold_offset()
                : vec3(0));
    auto m_rot = mat4(1.0);

    // TODO: fix so that X-coordinate is flipped according to side
    // X: to side/in towards player
    // Y: up/down
    // Z: in front of/behind player
    switch (hold_type) {
        case Item::HoldType::DIAGONAL:
            m_trans = math::translate(m_trans, vec3(-2, -3, 6));
            m_rot *= math::rotate(mat4(1.0), -math::PI_2, vec3(0, 1, 0));
            m_rot *= math::rotate(mat4(1.0), -math::PI_4, vec3(0, 0, 1));
            break;
        case Item::HoldType::TILE:
            m_trans = math::translate(m_trans, vec3(0, -3, -2));
            break;
        case Item::HoldType::ATTACHMENT_FLIPPED:
            m_trans = math::translate(m_trans, vec3(-2, -7, 2));
            m_rot *= math::rotate(mat4(1.0), math::PI, vec3(0, 0, 1));
            m_rot *= math::rotate(mat4(1.0), -math::PI_2, vec3(0, 1, 0));
            break;
        case Item::HoldType::ATTACHMENT:
        default:
            m_trans = math::translate(m_trans, vec3(-2, -3, 2));
            m_rot *= math::rotate(mat4(1.0), -math::PI_2, vec3(0, 1, 0));
            break;
    }

    m *= m_trans;

    const auto center_offset = vec3(8.0f, 8.0f, 0.0f) / 2.0f;
    m = math::translate(m, center_offset);
    m *= m_rot;
    m = math::translate(m, -center_offset);
    m = math::scale(m, vec3(SCALE));
    return m;
}

void ModelHumanoid::render(
    RenderContext &ctx,
    const RenderParams &params) const {
    const auto &entity = *params.entity;

    const auto passes =
        entity_render::adjust_passes(
            RENDER_FLAG_PASS_ALL_3D & ~RENDER_FLAG_PASS_TRANSPARENT,
            entity);

    const auto
        base_model = this->model_base(entity, *params.anim),
        lower_model = this->model_lower(entity, *params.anim, base_model);

    ctx.push(
        types::make_array(
            ModelPart {
                .entry = this->head,
                .model = this->model_head(entity, *params.anim, base_model),
                .passes = passes
            },
            ModelPart {
                .entry = this->body,
                .model = this->model_body(entity, *params.anim, base_model),
                .passes = passes
            }));

    for (const auto &side : Side::ALL) {
        ctx.push(
            types::make_array(
                ModelPart {
                    .entry = this->leg,
                    .model =
                        this->model_leg(
                            entity, *params.anim, lower_model, side),
                    .passes = passes
                },
                ModelPart {
                    .entry = this->arm,
                    .model =
                        this->model_arm(
                            entity, *params.anim, lower_model, side),
                    .passes = passes
                }));
    }

    if (const auto *entity_mob = dynamic_cast<const EntityMob*>(&entity)) {
        if (const auto held = entity_mob->held()) {
            held->item().icon().render(
                ctx,
                &*held,
                this->model_hold(
                    entity,
                    *params.anim,
                    base_model,
                    entity.side_hold(),
                    held));
        }
    }
}

f32 ModelHumanoid::swing(const EntityMob &entity) {
    const auto
        swing_scale = TICKS_PER_SECOND * 0.4f,
        swing =
            math::length(entity.pos_delta) > 0.0001f ?
                math::mod(
                    math::mod(global.time->ticks / swing_scale, swing_scale)
                        + (math::length(entity.velocity) * 0.016f),
                    1.0f)
                : 0.0f;
    return swing;
}

void ModelHumanoid::animate_pluck(AnimationState &state) {
    for (const auto &side : Side::ALL) {
        state.rot_arms[side] = {
            {
                { 4, 0.0f },
                { 20, -(math::PI_2 + math::PI_4) },
                { 20, 0.0f }
            },
            state.rot_arms[side].current()
        };
    }
}

void ModelHumanoid::animate_grab(
    AnimationState &state,
    Side::Enum side) {
    state.rot_arms[side] = {
        {
            { 4, 0.0f },
            { 10, -math::PI_2 },
            { 10, 0.0f }
        },
        state.rot_arms[side].current()
    };
}

void ModelHumanoid::animate_smash(AnimationState &state, Side::Enum side) {
    state.rot_arms[side] = {
         {
             { 4, 0.0f },
             { 8, math::PI },
             { 12, math::TAU },
         }
    };
}
