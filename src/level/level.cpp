#include "level/level.hpp"
#include "entity/entity.hpp"
#include "entity/entity_item.hpp"
#include "entity/entity_smash_particle.hpp"
#include "item/item_ref.hpp"
#include "item/drop_table.hpp"
#include "util/util.hpp"
#include "util/macros.hpp"
#include "serialize/serialize.hpp"
#include "serialize/context_level.hpp"
#include "global.hpp"

SERIALIZE_ENABLE()
DECL_SERIALIZER(Level)
DECL_PRIMITIVE_SERIALIZER(decltype(Level::chunks))
DECL_PRIMITIVE_SERIALIZER(decltype(Level::all_entities))

// run function f(Chunk&, const AABB2i&) for all overlapping chunk bounds in
// the current level
template <typename F>
static void f_chunk_bounded(
    const Level &level,
    const AABBi &bounds,
    F &&f) {
    const auto level_offset =
        AABB2i::compute(
            Level::to_offset(bounds.min.xz()),
            Level::to_offset(bounds.max.xz()));

    for (isize x = level_offset.min.x; x <= level_offset.max.x; x++) {
        for (isize z = level_offset.min.y; z <= level_offset.max.y; z++) {
            const auto offset = ivec2(x, z);

            if (!level.contains_chunk(offset)) {
                continue;
            }

            auto &chunk = level.chunk(offset);
            const auto
                box_chunk =
                    AABB2i(ivec2(0), Chunk::SIZE.xz() - 1),
                box_rel =
                    AABB2i(bounds.min.xz(), bounds.max.xz())
                        .translate(-chunk.offset_tiles.xz());

            if (!box_chunk.collides(box_rel)) {
                continue;
            }

            f(chunk, box_chunk.intersect(box_rel));
        }
    }
}

static void init_allocators(Level &level, Allocator *allocator) {
    level.allocator = BasicAllocator();
    level.entity_allocator =
        decltype(level.entity_allocator)(&level.allocator);
    level.item_metadata_allocator =
        decltype(level.item_metadata_allocator)(&level.allocator);
    level.chunk_allocator =
        decltype(level.chunk_allocator)(&level.allocator);

    level.poly_allocator = decltype(level.poly_allocator)(&level.allocator);
    level.poly_allocator
        .set<Entity>(
            [&level]() -> Allocator& { return level.entity_allocator; })
        .set<ItemMetadata>(
            [&level]() -> Allocator& { return level.item_metadata_allocator; })
        .set<Chunk>(
            [&level]() -> Allocator& { return level.chunk_allocator; });
}

static void init(Level &level) {
    level.raw = decltype(level.raw)(&level, &Chunk::raw);
    level.tiles = decltype(level.tiles)(&level, &Chunk::tiles);
    level.light = decltype(level.light)(&level, &Chunk::light);
    level.subtile = decltype(level.subtile)(&level, &Chunk::subtile);
    level.flags = decltype(level.flags)(&level, &Chunk::flags);
    level.ghost = decltype(level.ghost)(&level, &Chunk::ghost);
}

Level::Level(
    Allocator *allocator,
    usize depth,
    const ivec2 &size,
    std::function<void(Level&)> &&generate)
    : depth(depth),
      size(size) {
    init_allocators(*this, allocator);
    init(*this);

    this->chunks =
        List<alloc_ptr<Chunk>>(
            &this->allocator,
            this->size.x * this->size.y);

    // place chunks
    for (int x = 0; x < this->size.x; x++) {
        for (int z = 0; z < this->size.y; z++) {
            const auto offset = ivec2(x, z);
            this->chunks[Level::to_index(offset)] =
                make_alloc_ptr<Chunk>(this->chunk_allocator, *this, offset);
        }
    }

    // generate level
    generate(*this);
}

void Level::on_before_deserialize(SerializationContextLevel &ctx) {
    init_allocators(*this, ctx.base_allocator);
}

void Level::on_resolve(SerializationContextLevel &ctx) {
    init(*this);
}

void Level::update() {
    for (auto &chunk : this->chunks) {
        chunk->update();
    }
}

void Level::tick() {
    for (auto &chunk : this->chunks) {
        chunk->tick();
    }

    // remove all entities which are flagged for deletion
    for (auto &ptr : this->all_entities) {
        if (ptr) {
            auto &e = *ptr;
            if (e.should_destroy) {
                e.on_destroy();
                e.detach();
                this->all_entities[e.id].clear();
            } else {
                e.relocate();
            }
        }
    }
}

Entity *Level::spawn(
    alloc_ptr<Entity> &&_entity,
    const vec3 &pos) {
    // give this function ownership so entity is deleted if not added to main
    // array
    alloc_ptr<Entity> entity = std::move(_entity);

    entity->level = this;
    entity->pos = pos;

    if (!entity->check_spawn_collision()) {
        goto done;
    }

    if (const auto aabb = entity->aabb()) {
        auto colliders = global.tick_allocator.alloc_span<AABB>(256);
        auto entities =
            global.tick_allocator.alloc_span<Entity*>(
                256,
                Allocator::F_CALLOC);
        const auto [n_colliders, overflow] =
            this->colliders(
                std::span(colliders),
                Level::aabb_to_tile((*aabb).scale_center(2.0f)),
                nullptr,
                entities,
                [](const Entity &e) { return e.collides_with_entities(); });

        if (overflow) {
            WARN("overflow when spawning entity");
            return nullptr;
        }

        for (usize i = 0; i < n_colliders; i++) {
            const auto &box = colliders[i];
            const auto *other = entities[i];

            if ((!other
                    || (other->does_stop(*entity)
                        && entity->does_stop(*other)))
                    && box.collides(*aabb)) {
                WARN("could not spawn entity, collision {}, {}", box, *aabb);
                return nullptr;
            }
        }
    }

done:
    ASSERT(this->all_entities[entity->id].get() == nullptr);
    auto &ptr = (this->all_entities[entity->id] = std::move(entity));
    ptr->init();
    ptr->on_level_change();
    return ptr.get();
}

usize Level::num_entities(const AABBi &area) const {
    usize n = 0;
    f_chunk_bounded(
        *this, area,
        [&](Chunk &chunk, const AABB2i &box) {
            n += chunk.num_entities(
                box, [&area](const Entity &e) {
                    if (e.pos.y < area.min.y || e.pos.y > area.max.y) {
                        return false;
                    }

                    return true;
                });
        });
    return n;
}

std::tuple<usize, bool> Level::entities(
    std::span<Entity*> dest,
    const AABBi &area,
    std::optional<EntityFilterFn> filter) const {
    usize n = 0;
    bool overflow = false;

    f_chunk_bounded(
        *this, area,
        [&](Chunk &chunk, const AABB2i &box) {
            if (overflow) {
                return;
            }

            const auto n_entities = chunk.num_entities(box);
            auto entities =
                global.tick_allocator.alloc_span<Entity*>(
                    n_entities,
                    Allocator::F_CALLOC);

            const auto [n_entities_ex, overflow_c] =
                chunk.get_entities(entities, box);
            ASSERT(n_entities == n_entities_ex);
            overflow |= overflow_c;

            for (auto *e : entities) {
                if (filter && !(*filter)(*e)) {
                    continue;
                }

                // cull on y axis
                if (e->pos.y < area.min.y || e->pos.y > area.max.y) {
                    continue;
                }

                if (n >= dest.size()) {
                    overflow = true;
                    return;
                }

                dest[n++] = e;
            }
        });

    return std::make_tuple(n, overflow);
}

std::tuple<usize, bool> Level::entities(
    std::span<Entity*> dest,
    const ivec2 &xz,
    std::optional<EntityFilterFn> filter) const {
    Chunk *c;
    if (!(c = this->chunkp(Level::to_offset(xz)))) {
        return std::make_tuple(0, false);
    }

    const auto &entities =
        c->get_xz_entities(Level::to_chunk_pos(ivec3(xz.x, 0, xz.y)).xz());

    usize n = 0;
    bool overflow = false;

    for (auto *e : entities) {
        if (filter && !(*filter)(*e)) {
            continue;
        }

        if (n >= dest.size()) {
            overflow = true;
            break;
        }

        dest[n++] = e;
    }

    return std::make_tuple(n, overflow);
}

std::tuple<usize, bool> Level::tile_colliders(
    std::span<AABB> dest,
    const AABBi &area,
    const Entity *for_entity) const {
    usize n = 0;

    for (isize x = area.min.x; x <= area.max.x; x++) {
        for (isize y = area.min.y; y <= area.max.y; y++) {
            for (isize z = area.min.z; z <= area.max.z; z++) {
                const auto pos = ivec3(x, y, z);

                const auto proxy = this->raw[pos];

                const TileId tile_id = Chunk::TileData::from(proxy);
                if (tile_id == 0) {
                    continue;
                }

                const auto &tile = Tiles::get()[tile_id];

                if (for_entity
                        && !tile.collides(*this, pos, *for_entity)) {
                    continue;
                }

                const auto box = OPT_OR_CONT(tile.aabb(*this, pos));

                if (n >= dest.size()) {
                    return std::make_tuple(n, true);
                }

                dest[n++] = box;
            }
        }
    }

    return std::make_tuple(n, false);
}

std::tuple<usize, bool> Level::entity_colliders(
    std::span<AABB> dest,
    const AABBi &area,
    const Entity *for_entity,
    std::optional<std::span<Entity*>> dest_entities,
    std::optional<EntityFilterFn> filter) const {
    usize n = 0;

    std::array<Entity*, 256> entities;
    const auto [n_entities, overflow] =
        this->entities(std::span(entities), area, filter);

    for (auto *e : std::span(entities).subspan(0, n_entities)) {
        if (e == for_entity) {
            continue;
        }

        const auto aabb = OPT_OR_CONT(e->aabb());

        if (n >= dest.size()) {
            return std::make_tuple(n, true);
        }

        dest[n] = aabb;

        if (dest_entities) {
            (*dest_entities)[n] = e;
        }

        n++;
    }

    return std::make_tuple(n, overflow);
}

std::tuple<usize, bool> Level::colliders(
    std::span<AABB> dest,
    const AABBi &area,
    const Entity *for_entity,
    std::optional<std::span<Entity*>> dest_entities,
    std::optional<EntityFilterFn> filter,
    usize collider_types) const {
    if (dest_entities) {
        ASSERT(dest.size() == dest_entities->size());
    }

    usize n = 0;

    if (collider_types & COLLIDER_TYPE_TILE) {
        const auto [n_tile, overflow_t] =
            this->tile_colliders(dest, area, for_entity);
        n += n_tile;

        if (overflow_t) {
            return std::make_tuple(n, true);
        }
    }

    if (collider_types & COLLIDER_TYPE_ENTITY) {
        const auto [n_entity, overflow_e] =
            this->entity_colliders(
                dest.subspan(n), area, for_entity,
                dest_entities ?
                    std::make_optional((*dest_entities).subspan(n))
                    : std::nullopt,
                filter);
        n += n_entity;

        if (overflow_e) {
            return std::make_tuple(n, true);
        }
    }

    return std::make_tuple(n, false);
}

std::tuple<usize, bool> Level::offsets_with_flags(
    std::span<ivec3> dest,
    const AABBi &area,
    Chunk::FlagsType flags) const {
    usize n = 0;
    bool overflow = false;
    f_chunk_bounded(
        *this, area,
        [&](Chunk &chunk, const AABB2i &box) {
            if (overflow) {
                return;
            }

            const auto [n_c, overflow_c] =
                chunk.offsets_with_flags(
                    dest.subspan(n), box, flags);

            overflow |= overflow_c;

            // convert chunk offsets to level offsets
            for (usize i = n; i < n + n_c; i++) {
                dest[i] += chunk.offset_tiles;
            }

            n += n_c;
        });

    return std::make_tuple(n, overflow);
}

bool Level::collides(
    const vec3 &pos,
    const Entity *for_entity,
    std::optional<EntityFilterFn> filter,
    usize collider_types) const {
    const auto boxes = global.tick_allocator.alloc_span<AABB>(64);

    const auto pos_i = Level::to_tile(pos);
    const auto [_, overflow] =
        this->colliders(
            boxes,
            AABBi(pos_i, pos_i),
            for_entity,
            std::nullopt,
            filter,
            collider_types);

    if (overflow) {
        return true;
    }

    for (const auto &box : boxes) {
        if (box.contains(pos)) {
            return true;
        }
    }

    return false;
}

std::tuple<usize, bool> Level::lights(
    std::span<Light> dest,
    const AABBi &area) const {
    usize n = 0;
    bool overflow = false;

    f_chunk_bounded(
        *this, area,
        [&](Chunk &chunk, const AABB2i &box) {
            if (overflow) {
                return;
            }

            const auto [m, overflow_c] = chunk.lights(dest.subspan(n), box);
            n += m;
            overflow |= overflow_c;
        });

    return std::make_tuple(n, overflow);
}

std::optional<std::tuple<usize, TileId>> Level::topmost_tile(const ivec2 &xz) {
    for (isize y = Chunk::SIZE.y - 1; y >= 0; y--) {
        TileId t;
        if ((t = this->tiles[{ xz.x, y, xz.y }]) != 0) {
            return std::make_tuple(y, t);
        }
    }

    return std::nullopt;
}

bool Level::visible(const ivec3 &pos) const {
    usize n = 0;
    for (const auto &d : Direction::ALL) {
        const auto pos_n = pos + Direction::to_ivec3(d);
        const auto tile = this->tiles[pos_n];
        if (tile != 0 && Tiles::get()[tile].solid(*this, pos_n)) {
            n++;
        }
    }
    return n != Direction::COUNT;
}

bool Level::can_place_tile(const Tile &tile, const ivec3 &pos) const {
    if (!this->contains(pos)) {
        return false;
    }

    const auto empty = this->tiles[pos] == 0;
    const auto aabb = OPT_OR_RET(tile.aabb(*this, pos), empty);

    const auto box_check = AABBi(pos - 1, pos + 1);
    const auto n_entities = this->num_entities(box_check);
    const auto boxes = global.tick_allocator.alloc_span<AABB>(n_entities);
    const auto entities =
        global.tick_allocator.alloc_span<Entity*>(
            n_entities,
            Allocator::F_CALLOC);
    this->entity_colliders(boxes, box_check, nullptr, entities);

    for (usize i = 0; i < boxes.size(); i++) {
        if (entities[i]
                && aabb.collides(boxes[i])
                && tile.collides(*this, pos, *entities[i])) {
            return false;
        }
    }

    return true;
}

void Level::destroy_tile(
    const ivec3 &pos,
    EntityRef entity,
    ItemRef item) {
    const auto &tile = Tiles::get()[this->tiles[pos]];
    this->tiles[pos] = 0;

    // drop items
    EntityItem::drop(
        *this,
        Level::to_tile_center(pos),
        tile.drops(*this, pos));

    // particles
    EntityParticle::spawn<EntitySmashParticle>(
        *this,
        Level::to_tile_center(pos),
        5, 10,
        tile.renderer().colors());
}
