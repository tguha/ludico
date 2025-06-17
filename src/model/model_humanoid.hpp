#pragma once

#include "model/model_entity.hpp"
#include "voxel/voxel_multi_mesh.hpp"
#include "gfx/keyframe.hpp"
#include "gfx/specular_textures.hpp"
#include "util/side.hpp"
#include "global.hpp"

struct EntityMob;
struct EntityRenderer;
struct RenderContext;
struct ItemRef;

struct ModelHumanoid : public ModelEntity {
    struct Textures {
        struct {
            std::array<TextureArea, Direction::COUNT> areas_tex;
            std::array<TextureArea, Direction::COUNT> areas_spec;

            inline auto tex_opt() const {
                return collections::copy_to<
                    std::array<std::optional<TextureArea>, Direction::COUNT>>(
                        this->areas_tex);
            }

            inline auto spec_opt() const {
                return collections::copy_to<
                    std::array<std::optional<TextureArea>, Direction::COUNT>>(
                        this->areas_spec);
            }
        } head, body, arm, leg;

        static inline Textures from_default_spritesheet(
            const MultiTexture &sprites) {
            return Textures {
                .head = {
                    {
                        sprites[{0, 0}],
                        sprites[{1, 0}],
                        sprites[{1, 0}],
                        sprites[{1, 0}],
                        sprites[{2, 0}],
                        sprites[{3, 0}]
                    },
                    Direction::expand_directional_array<
                        std::array<TextureArea, Direction::COUNT>>(
                            SpecularTextures::instance().min())
                },
                .body = {
                    {
                        sprites[{0, 1}],
                        sprites[{1, 1}],
                        sprites[{2, 1}],
                        sprites[{2, 1}],
                        sprites[{3, 1}],
                        sprites[{4, 1}],
                    },
                    Direction::expand_directional_array<
                        std::array<TextureArea, Direction::COUNT>>(
                            SpecularTextures::instance().min())
                },
                .arm = {
                    {
                        sprites[{0, 2}],
                        sprites[{0, 2}],
                        sprites[{0, 2}],
                        sprites[{0, 2}],
                        sprites[{1, 2}],
                        sprites[{2, 2}],
                    },
                    Direction::expand_directional_array<
                        std::array<TextureArea, Direction::COUNT>>(
                            SpecularTextures::instance().min())
                },
                .leg = {
                    {
                        sprites[{0, 3}],
                        sprites[{0, 3}],
                        sprites[{0, 3}],
                        sprites[{0, 3}],
                        sprites[{1, 3}],
                        sprites[{2, 3}],
                    },
                    Direction::expand_directional_array<
                        std::array<TextureArea, Direction::COUNT>>(
                            SpecularTextures::instance().min())
                }
            };
        }
    };

    struct AnimationState {
        // head rotation along y-axis
        KeyframeList<f32> rot_head;

        // arm rotation along x-axis
        std::array<KeyframeList<f32>, Side::COUNT> rot_arms;

        // leg rotation along x-axis
        std::array<KeyframeList<f32>, Side::COUNT> rot_legs;

        // body rotation along x-axis (excludes head!)
        KeyframeList<f32> tilt_body;

        // arm/leg swing in [0, 1]
        std::optional<f32> swing;
    };

    struct RenderParams {
        EntityRenderer *entity_renderer;
        const EntityMob *entity;
        const AnimationState *anim;
    };

    static constexpr auto
        HEAD_OFFSET_PX = ivec3(0, 10, 0),
        BODY_OFFSET_PX = ivec3(0, 3, 2);

    // offset of head from model base position
    ivec3 head_offset_px;

    // offset of body from model base position
    ivec3 body_offset_px;

    VoxelMultiMesh mesh;
    const VoxelMultiMeshEntry *head, *body, *arm, *leg;

    explicit ModelHumanoid(
        const MultiTexture &texture,
        const Textures &textures);

    void render(RenderContext &ctx, const RenderParams &params) const;

    AABB aabb() const override;

    AABB entity_aabb(const Entity &entity) const override;

    f32 rot_swing_arm(
        const EntityMob &entity,
        const AnimationState &anim,
        Side::Enum side) const;

    f32 rot_swing_leg(
        const EntityMob &entity,
        const AnimationState &anim,
        Side::Enum side) const;

    // model matrices
    mat4 model_base(
        const EntityMob &entity,
        const AnimationState &anim) const;

    mat4 model_lower(
        const EntityMob &entity,
        const AnimationState &anim,
        const mat4 &base) const;

    mat4 model_head(
        const EntityMob &entity,
        const AnimationState &anim,
        const mat4 &base) const;

    mat4 model_body(
        const EntityMob &entity,
        const AnimationState &anim,
        const mat4 &base) const;

    mat4 model_arm(
        const EntityMob &entity,
        const AnimationState &anim,
        const mat4 &base,
        Side::Enum side) const;

    mat4 model_leg(
        const EntityMob &entity,
        const AnimationState &anim,
        const mat4 &base,
        Side::Enum side) const;

    mat4 model_hold(
        const EntityMob &entity,
        const AnimationState &anim,
        const mat4 &base,
        Side::Enum side,
        ItemRef item) const;

    // gets swing from [0, 1] for this entity according to velocity
    static f32 swing(const EntityMob &entity);

    // animates a "pluck" (both arms go quickly up)
    static void animate_pluck(AnimationState &state);

    // animates a "grab" (holding arm quickly extends)
    static void animate_grab(
        AnimationState &state,
        Side::Enum side = Side::LEFT);

    // animates a "smash" (holding arm quickly rotates fully)
    static void animate_smash(
        AnimationState &state,
        Side::Enum side = Side::LEFT);
};
