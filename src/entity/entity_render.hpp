#pragma once

#include "entity/entity.hpp"
#include "gfx/util.hpp"
#include "gfx/game_camera.hpp"
#include "gfx/renderer.hpp"
#include "state/state_game.hpp"
#include "constants.hpp"
#include "global.hpp"

struct GenericMultiMesh;
struct GenericMultiMeshEntry;
struct MultiMeshInstanceBuffer;
struct RenderGroup;

// misc. entity rendering helper functions
namespace entity_render {
// adjust passes for rendering specified entity
u8 adjust_passes(u8 passes, const Entity &entity);

// adjust flags for rendering specified entity
u8 adjust_flags(u8 flags, const Entity &entity);

// prepare a group to render an entity
void configure_group(RenderGroup &group, const Entity &entity);

// get base model matrix to render entities
mat4 model_base(const Entity &entity);

// get billboarded model matrix to render entities
mat4 model_billboard(const Entity &entity);
}
