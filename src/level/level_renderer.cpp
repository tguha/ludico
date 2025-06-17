#include "level/level_renderer.hpp"
#include "level/chunk_renderer.hpp"
#include "tile/tile_renderer.hpp"
#include "entity/entity.hpp"
#include "level/level.hpp"
#include "gfx/renderer.hpp"
#include "gfx/game_camera.hpp"
#include "gfx/render_context.hpp"
#include "state/state_game.hpp"
#include "constants.hpp"
#include "global.hpp"

void LevelRenderer::render() {
    const auto bounds = global.game->camera->render_bounds();

    this->instanced_extras_renderers.clear();

    // get rid of those that are no longer valid chunk renderers
    for (auto it = this->chunk_renderers.begin();
         it != this->chunk_renderers.end();) {
        auto &[offset, _] = *it;

        if (!this->level->contains_chunk(offset)) {
            this->chunk_renderers.erase(it++);
        } else {
            it++;
        }
    }

    // frustum cull entities
    const auto bounds_e =
        AABBi(
            ivec3(bounds.min.x, 0, bounds.min.y),
            ivec3(bounds.max.x, Chunk::SIZE.y, bounds.max.y));
    const auto n_entities_alloc = this->level->num_entities(bounds_e);

    auto entities =
        global.frame_allocator.alloc_span<Entity*>(
            n_entities_alloc,
            Allocator::F_CALLOC);
    const auto [_, overflow_e] =
        this->level->entities(entities, bounds_e);
    ASSERT(!overflow_e);

    // keep entities
    this->frame_entities = entities;

    // gather tiles with extras flag
    auto tiles = global.frame_allocator.alloc_span<ivec3>(bounds_e.volume());
    const auto [n_tiles, overflow_t] =
        this->level->offsets_with_flags(
            tiles, bounds_e, Chunk::TF_RENDER_EXTRAS);
    ASSERT(!overflow_t);

    // filter into visible tiles
    auto tiles_visible =
        global.frame_allocator.alloc_span<ivec3>(n_tiles);

    usize n = 0;
    for (const auto &pos : tiles.subspan(0, n_tiles)) {
        if (global.game->camera->frustum
                .contains(Level::to_tile_center(pos))
                && this->level->visible(pos)) {
            tiles_visible[n++] = pos;

            // check if renderer has instanced extras which need to be
            // renderered
            const auto &tile = Tiles::get()[this->level->tiles[pos]];
            const auto &renderer = tile.renderer();
            if (renderer.has_instanced_extras()) {
                this->instanced_extras_renderers.insert(&renderer);
            }
        }
    }

    this->frame_extra_tiles = tiles_visible.subspan(0, n);

    // traverse entity renderers, construct context
    this->frame_ctx =
        global.frame_allocator.alloc<RenderContext>(
            &global.frame_allocator);

    for (auto *e : this->frame_entities) {
        e->render(*this->frame_ctx);
    }

    // render chunks in bounds
    const auto bounds_c =
        bounds.transform(
            [](ivec2 tile) {
                return Level::to_offset(tile);
            });

    for (isize x = bounds_c.min.x; x <= bounds_c.max.x; x++) {
        for (isize z = bounds_c.min.y; z <= bounds_c.max.y; z++) {
            ChunkRenderer *cr = nullptr;
            const auto offset = ivec2(x, z);

            // create chunk renderer if it does not already exist
            if (this->chunk_renderers.contains(offset)) {
                cr = &this->chunk_renderers.at(offset);
            } else if (this->level->contains_chunk(offset)) {
                auto &chunk = this->level->chunk(offset);
                auto res = this->chunk_renderers.emplace(
                    chunk.offset,
                    ChunkRenderer(chunk));
                cr = &res.first->second;
            }

            if (cr) {
                cr->render(*this->frame_ctx);
            }
        }
    }


    // render_extras for all frame extras tiles
    for (const auto &pos : this->frame_extra_tiles) {
        const auto &tile = Tiles::get()[this->level->tiles[pos]];
        tile.renderer().render_extras(
            *this->frame_ctx,
            this->level,
            pos);
    }

    // render instanced extras
    for (const auto *r : this->instanced_extras_renderers) {
        r->render_instanced_extras(*this->frame_ctx, this->level);
    }

    this->frame_ctx->prepare();
}

void LevelRenderer::pass(RenderState render_state) {
    // render everything in render context
    this->frame_ctx->pass(render_state);
}
