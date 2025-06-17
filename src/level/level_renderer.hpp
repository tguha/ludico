#pragma once

#include "util/types.hpp"
#include "util/util.hpp"
#include "level/chunk_renderer.hpp"

struct Level;
struct Entity;
struct TileRenderer;
struct RenderContext;

struct LevelRenderer {
    Level *level;

    // TODO: more efficient storage
    std::unordered_map<ivec2, ChunkRenderer> chunk_renderers;

    explicit LevelRenderer(Level &level)
        : level(&level) { }

    void render();

    void pass(RenderState render_state);
private:
    RenderContext *frame_ctx = nullptr;
    std::span<Entity*> frame_entities;
    std::span<ivec3> frame_extra_tiles;

    // TODO: more efficient storage
    std::unordered_set<const TileRenderer*> instanced_extras_renderers;
};
