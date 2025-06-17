#pragma once

#include "util/types.hpp"
#include "util/util.hpp"
#include "util/optional.hpp"
#include "util/basic_allocator.hpp"
#include "util/pool_allocator.hpp"
#include "util/elastic_pool_allocator.hpp"
#include "util/id_based_allocator.hpp"
#include "util/alloc_ptr.hpp"
#include "util/list.hpp"
#include "level/chunk.hpp"
#include "level/light.hpp"
#include "levelgen/gen.hpp"
#include "item/item_metadata.hpp"
#include "item/util.hpp"
#include "entity/entity_ref.hpp"
#include "entity/util.hpp"

struct ItemStack;
struct Archive;

struct Level {
    // frequency of random ticks/minute (average)
    static constexpr usize RANDOM_TICKS_PER_MINUTE = 30;

    // maximum number of entities in a level
    static constexpr usize MAX_ENTITIES = 4096;

    enum ColliderType : u8 {
        COLLIDER_TYPE_ENTITY = 1 << 0,
        COLLIDER_TYPE_TILE = 1 << 1
    };

    // proxy for access to level data through chunk data proxy
    template <typename T>
    struct LevelDataAccess final {
        Level *level;
        T Chunk::*member;

        LevelDataAccess() = default;
        LevelDataAccess(
            Level *level,
            T Chunk::*member)
            : level(level), member(member) { };

        // returns a SafeProxy, use direct chunk access for better speed
        inline auto operator[](const ivec3 &pos) {
            if (!Level::in_bounds(pos)) {
                return T::ZERO;
            }

            const auto offset = Level::to_offset(pos.xz());
            const auto chunkp = this->level->chunkp(offset);
            return chunkp ?
                (chunkp->*this->member).safe(Level::to_chunk_pos(pos))
                : T::ZERO;
        }

        inline const typename T::SafeProxy operator[](const ivec3 &pos) const {
            return (*const_cast<LevelDataAccess*>(this))[pos];
        }

        // does no bounds checking and returns a Proxy directly
        inline auto unsafe(const ivec3 &pos) {
            const auto offset = Level::to_offset(pos.xz());
            return
                (this->level->chunk(offset).*this->member)
                    [Level::to_chunk_pos(pos)];
        }


        inline const typename T::Proxy unsafe(const ivec3 &pos) const {
            return const_cast<LevelDataAccess*>(this)->unsafe(pos);
        }
    };

    // TODO: change to free-list
    // basic malloc-backed allocator
    [[SERIALIZE_IGNORE]]
    BasicAllocator allocator;

    // allocators
    [[SERIALIZE_IGNORE]]
    PoolAllocator<1024, 4096> entity_allocator;

    [[SERIALIZE_IGNORE]]
    ElasticPoolAllocator<sizeof(ItemMetadata) * 4, 256> item_metadata_allocator;

    [[SERIALIZE_IGNORE]]
    PoolAllocator<sizeof(Chunk), 64> chunk_allocator;

    // universal allocator for all of the above types
    [[SERIALIZE_IGNORE]]
    IDBasedAllocator poly_allocator;

    // data access proxies
    [[SERIALIZE_IGNORE]] LevelDataAccess<decltype(Chunk::raw)> raw;
    [[SERIALIZE_IGNORE]] LevelDataAccess<decltype(Chunk::tiles)> tiles;
    [[SERIALIZE_IGNORE]] LevelDataAccess<decltype(Chunk::light)> light;
    [[SERIALIZE_IGNORE]] LevelDataAccess<decltype(Chunk::subtile)> subtile;
    [[SERIALIZE_IGNORE]] LevelDataAccess<decltype(Chunk::flags)> flags;
    [[SERIALIZE_IGNORE]] LevelDataAccess<decltype(Chunk::ghost)> ghost;

    // chunk data
    List<alloc_ptr<Chunk>> chunks;

    // list of entities
    // SERIALIZE_IGNORE'd because entities are de/serialized manually in Chunk
    [[SERIALIZE_IGNORE]]
    std::array<alloc_ptr<Entity>, MAX_ENTITIES> all_entities;

    // depth of level (positive, increasing as level lowers)
    usize depth;

    // size of level, in chunks
    ivec2 size;

    Level(
        Allocator *allocator,
        usize depth,
        const ivec2 &size,
        std::function<void(Level&)> &&generate);

    Level() = default;
    Level(const Level &other) = delete;
    Level(Level &&other) = default;
    Level &operator=(const Level &other) = delete;
    Level &operator=(Level &&other) = default;

    void on_before_deserialize(SerializationContextLevel &ctx);

    void on_resolve(SerializationContextLevel &ctx);

    std::optional<EntityId> next_entity_id() {
        const auto start = this->_next_entity_id;
        auto id = start;
        do {
            id = (id + 1) % MAX_ENTITIES;
            if (id == 0) {
                id++;
            }
        } while (this->all_entities[id] && id != start);

        if (id == start) {
            WARN("out of entity IDs");
            return std::nullopt;
        }

        return id;
    }

    inline alloc_ptr<Entity> &entity_ptr(EntityId id) {
        return this->all_entities[id];
    }

    inline const alloc_ptr<Entity> &entity_ptr(EntityId id) const {
        return this->all_entities[id];
    }

    inline Entity *entity_from_id(EntityId id) const {
        return this->all_entities[id].get();
    }

    inline ItemStackId next_item_stack_id() {
        return this->_next_item_stack_id++;
    }

    inline ItemContainerId next_item_container_id() {
        return this->_next_item_container_id++;
    }

    void update();

    void tick();

    // loads an already allocated entity into this level
    template <typename E>
    IEntityRef<E> load(alloc_ptr<E> &&e) {
        ASSERT(!this->all_entities[e->id]);
        ASSERT(e.allocator() == &this->entity_allocator);

        // NOTICE that we are not calling on_level_changed for these entities,
        // or anything of the sort - this is handled by the entities
        // *themselves* when they are on_resolve'd by the serializer
        auto &ptr = (this->all_entities[e->id] = std::move(e));
        return dynamic_cast<E*>(ptr.get());
    }

    // tries to spawn a positioned entity into this level, returns nullptr if it
    // could not (collision, etc.)
    template <typename E, typename ...Args>
    IEntityRef<E> spawn(const vec3 &pos, Args&& ...args) {
        const auto id = OPT_OR_RET(this->next_entity_id(), nullptr);
        auto e =
            make_alloc_ptr<E>(
                this->entity_allocator,
                std::forward<Args>(args)...);
        e->id = id;
        return dynamic_cast<E*>(this->spawn(std::move(e), pos));
    }

    // tries to spawn an entity at the specified XZ coordinate into this level
    // returns nullptr if it could not
    template <typename E, typename ...Args>
    IEntityRef<E> spawn_at_xz(const ivec2 &xz, Args&& ...args) {
        const auto id = OPT_OR_RET(this->next_entity_id(), nullptr);
        auto e =
            make_alloc_ptr<E>(
                this->entity_allocator,
                std::forward<Args>(args)...);
        e->id = id;
        const auto [y, _] = OPT_OR_RET(this->topmost_tile(xz), nullptr);
        const auto pos = vec3(xz.x, y + 1.005f, xz.y);
        return dynamic_cast<E*>(this->spawn(std::move(e), pos));
    }

    // tries to spawn a positioned entity into this level, returns nullptr if it
    // could not (collision, etc.). otherwise returns entity pointer.
    Entity *spawn(alloc_ptr<Entity> &&entity, const vec3 &pos);

    // get number of entities in area
    usize num_entities(const AABBi &area) const;

    // get entities in area, optionally filtering
    std::tuple<usize, bool> entities(
        std::span<Entity*> dest,
        const AABBi &area,
        std::optional<EntityFilterFn> filter = std::nullopt) const;

    // get entities in XZ column, optionally filtering
    std::tuple<usize, bool> entities(
        std::span<Entity*> dest,
        const ivec2 &xz,
        std::optional<EntityFilterFn> filter = std::nullopt) const;

    // get tile colliders in area, optionally with regards to a specific entity
    std::tuple<usize, bool> tile_colliders(
        std::span<AABB> dest,
        const AABBi &area,
        const Entity *for_entity = nullptr) const;

    // get entity colliders in area, optionally with regards to a specific
    // entity. if entity is specified, then it is excluded from the collider
    // search. entities can optionally be filtered.
    std::tuple<usize, bool> entity_colliders(
        std::span<AABB> dest,
        const AABBi &area,
        const Entity *for_entity = nullptr,
        std::optional<std::span<Entity*>> dest_entities = std::nullopt,
        std::optional<EntityFilterFn> filter = std::nullopt) const;

    // get colliders in area, optionally with regards to a specific entity
    std::tuple<usize, bool> colliders(
        std::span<AABB> dest,
        const AABBi &area,
        const Entity *for_entity = nullptr,
        std::optional<std::span<Entity*>> dest_entities = std::nullopt,
        std::optional<EntityFilterFn> filter = std::nullopt,
        usize collider_types = COLLIDER_TYPE_ENTITY | COLLIDER_TYPE_TILE) const;

    // gets tile offsets in specified area which match the specified flags
    std::tuple<usize, bool> offsets_with_flags(
        std::span<ivec3> dest,
        const AABBi &area,
        Chunk::FlagsType flags) const;

    // returns true if the specified points collides
    bool collides(
        const vec3 &pos,
        const Entity *for_entity = nullptr,
        std::optional<EntityFilterFn> filter = std::nullopt,
        usize collider_types = COLLIDER_TYPE_ENTITY | COLLIDER_TYPE_TILE) const;

    // get lights in area
    std::tuple<usize, bool> lights(
        std::span<Light> dest, const AABBi &area) const;

    // find topmost tile at xz column
    std::optional<std::tuple<usize, TileId>> topmost_tile(const ivec2 &xz);

    // returns true if the tile at the specified position is possibly visible
    bool visible(const ivec3 &pos) const;

    // returns true if tile can be placed (does not collide, etc.) in specified
    // position
    bool can_place_tile(const Tile &tile, const ivec3 &pos) const;

    // destroy a tile (with drops, particles, etc.)
    void destroy_tile(
        const ivec3 &pos,
        EntityRef entity,
        ItemRef item);

    // AABB in tile space
    inline AABB2i aabb_tile() const {
        return AABB2i(ivec2(0), this->size * Chunk::SIZE.xz());
    }

    // AABB in chunk space
    inline AABB2i aabb_chunk() const {
        const auto tile = this->aabb_tile();
        return AABB2i(
            tile.min / Chunk::SIZE.xz(), tile.max / Chunk::SIZE.xz());
    }

    // AABB in 3D space
    inline AABB aabb() const {
        const auto tile = this->aabb_tile();
        return AABB(
            vec3(tile.min.x, 0, tile.min.y),
            vec3(tile.max.x, Chunk::SIZE.y, tile.max.y));
    }

    // returns true if the level contains the chunk at the specified offset
    // AND it is loaded in (present in the level)
    inline bool contains_chunk(const ivec2 &offset) const {
        const auto index = this->to_index(offset);

        return index >= 0 && index < this->chunks.size() ?
            static_cast<bool>(this->chunks[index])
            : false;
    }

    // returns true if the position is loaded in the level
    inline bool contains(const ivec3 &pos) const {
        return
            Level::in_bounds(pos)
                && this->contains_chunk(Level::to_offset(pos));
    }

    // get a raw chunk pointer
    inline Chunk *chunkp(const ivec2 &offset) const {
        const auto index = this->to_index(offset);
        return index >= 0 && index < this->chunks.size() ?
            this->chunks[index].get() : nullptr;
    }

    // get a raw chunk reference (crashes if chunk is not present!)
    inline Chunk &chunk(const ivec2 &offset) const {
        return *this->chunks[this->to_index(offset)].get();
    }

    // raw chunk data access via level offset
    inline auto operator[](const ivec3 &pos) {
        auto *chunk =
            this->chunks[this->to_index(Level::to_offset(pos))].get();
        return chunk ? (*chunk)[Level::to_chunk_pos(pos)] : 0;
    }

    // chunk offset to index
    inline usize to_index(const ivec2 &offset) const {
        return
            (offset.x < 0
            || offset.y < 0
            || offset.x >= this->size.x
            || offset.y >= this->size.y) ?
                std::numeric_limits<usize>::max()
                : (offset.y * this->size.x) + offset.x;
    }

    // utility functions
    // level pos to chunk pos
    static inline ivec3 to_chunk_pos(ivec3 pos_l) {
        // (((pos % size) + size) % size) to deal with negatives correctly
        return pos_l % Chunk::SIZE;
    }

    // level pos to chunk offset (XZ)
    static inline ivec2 to_offset(const ivec2 &pos_l) {
        return pos_l / Chunk::SIZE.xz();
    }

    // level pos to chunk offset
    static inline ivec2 to_offset(const ivec3 &pos_l) {
        return pos_l.xz() / Chunk::SIZE.xz();
    }

    static inline vec3 to_tile_center(const ivec3 &pos) {
        return vec3(pos) + 0.5f;
    }

    // float to tile pos
    static inline ivec3 to_tile(const vec3 &pos_f) {
        return ivec3(math::floor(pos_f));
    }

    // float to tile pos (XZ)
    static inline ivec2 to_tile(const vec2 &pos_f) {
        return ivec2(math::floor(pos_f));
    }

    static inline bool in_bounds(const ivec3 &pos) {
        return pos.x >= 0 && pos.z >= 0 && pos.y >= 0 && pos.y < Chunk::SIZE.y;
    }

    // round AABB to tile coordinates
    static inline auto aabb_to_tile(const AABB &box) {
        auto min = box.min, max = box.max;
        min.y = math::clamp<f32>(min.y, 0, Chunk::SIZE.y);
        max.y = math::clamp<f32>(max.y, 0, Chunk::SIZE.y);
        return AABBi(
            Level::to_tile(math::floor(min)),
            Level::to_tile(math::ceil(max)));
    }

private:
    EntityId _next_entity_id = 1;
    ItemStackId _next_item_stack_id = 1;
    ItemContainerId _next_item_container_id = 1;
};
