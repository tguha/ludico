#pragma once

#include "util/types.hpp"
#include "util/util.hpp"
#include "util/alloc_ptr.hpp"
#include "util/bump_allocator.hpp"
#include "util/moveable.hpp"
#include "entity/entity_ref.hpp"
#include "state.hpp"

struct GameCamera;
struct Sun;
struct OcclusionMap;
struct Level;
struct LevelRenderer;
struct UIHUD;
struct EntityPlayer;

struct StateGame : public State {
    // allocator tied to game state lifetime
    BumpAllocator allocator;
    alloc_ptr<GameCamera> camera;
    alloc_ptr<Sun> sun;
    alloc_ptr<OcclusionMap> occlusion_map;
    alloc_ptr<LevelRenderer> level_renderer;
    alloc_ptr<Level> level;
    UIHUD *hud;

    // current player
    EntityPlayer *player;
    EntityRef camera_entity;

    StateGame() : State(STATE_GAME) {}

    void init() override;
    void destroy() override;
    void tick() override;
    void update() override;
    void render() override;

    void render_3d(RenderState render_state);
    void render_ui(RenderState render_state);
};
