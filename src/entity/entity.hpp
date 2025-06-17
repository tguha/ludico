#pragma once

#include "util/types.hpp"
#include "util/util.hpp"
#include "util/lerp_dir.hpp"
#include "util/direction.hpp"
#include "util/aabb.hpp"
#include "serialize/annotations.hpp"
#include "entity/entity_ref.hpp"
#include "entity/util.hpp"
#include "gfx/util.hpp"

struct SerializationContextLevel;
struct Level;
struct Chunk;
struct ItemStack;
struct ItemContainer;
struct ItemRef;
struct InteractionAffordance;
struct InteractionResult;
struct RenderContext;
enum class InteractionKind : u8;

template <usize N>
struct NLightArray;
using LightArray = NLightArray<4>;

struct Entity {
    static constexpr auto GRAVITY = vec3(0.0f, -0.0056f, 0.0f);

    // offset when spawning entities so as to not collide with underlying tile
    static constexpr auto COLLISION_OFFSET = vec3(0.0f, 0.001f, 0.0f);

    // unique id
    EntityId id = NO_ENTITY;

    // level containing this entity
    [[SERIALIZE_BY_CTX(SERIALIZE_IGNORE, SerializationContextLevel::get_level)]]
    Level *level = nullptr;

    // chunk containing entity
    [[SERIALIZE_IGNORE]]
    Chunk *chunk = nullptr;

    // entity positions
    vec3 pos, last_pos, pos_delta, center;

    // tile positions
    ivec3 tile, center_tile, last_tile;

    // last tick where position was meaningfully changed
    usize last_move_ticks;

    // movement velocity
    vec3 velocity;

    // drag on velocity
    f32 drag = 1.0f;

    // collision restitution, nullopt if this entity does not bounce at all
    std::optional<f32> restitution = std::nullopt;

    // when set to true, entity is removed on next tick
    bool should_destroy = false;

    // ticks when entity was spawned
    usize spawned_tick = 0;

    static constexpr auto DIRECTION_LERP_TICKS = 6;
    LerpDir direction = LerpDir(DIRECTION_LERP_TICKS);

    Entity() = default;
    Entity(Entity&&) = default;
    Entity(const Entity&) = delete;
    Entity &operator=(Entity&&) = default;
    Entity &operator=(const Entity&) = delete;
    virtual ~Entity() = default;

    void on_resolve(SerializationContext &ctx);

    // called AFTER spawn, once added to level
    virtual void init();

    virtual void destroy() {
        this->should_destroy = true;
    }

    virtual std::string name() const { return "(ERR)"; }

    virtual std::string locale_name() const { return "(ERR)"; }

    virtual void on_level_change();

    virtual void tick();

    virtual void relocate();

    virtual void update() {}

    virtual void render(RenderContext&) {}

    // NOTE: entities compare ONLY on their id
    inline auto operator<=>(const Entity &other) const {
        return this->id <=> other.id;
    }

    // NOTE: entities compare ONLY on their id
    inline bool operator==(const Entity &other) const {
        return ((*this) <=> other) == 0;
    }

    // detaches entity from current level and chunk
    void detach();

    // called when entity is destroyed
    virtual void on_destroy() {}

    // returns true if entity is on ground
    bool grounded() const;

    // tries to move entity by movement, returns amount moved
    virtual vec3 move(const vec3 &movement);

    // returns this entity's inventory if present
    virtual ItemContainer *inventory() { return nullptr; }

    // returns const pointer to entity's inventory if present
    virtual const ItemContainer *inventory() const {
        return const_cast<Entity*>(this)->inventory();
    }

    // called on pickup of specified item
    virtual void on_pickup(const ItemStack &stack, EntityRef item);

    // checks if this entity can be interacted with by other
    virtual InteractionAffordance can_interact_with_this(
        InteractionKind kind, const Entity &other, ItemRef item) const;

    // checks if this entity can interact with target
    virtual InteractionAffordance can_interact(
        InteractionKind kind,
        const Entity &target,
        ItemRef item) const;

    // tries to interact other with this as target
    virtual InteractionResult interacted(
        InteractionKind kind,
        Entity &other,
        ItemRef item);

    // tries to interact with target
    virtual InteractionResult interact(
        InteractionKind kind,
        Entity &target,
        ItemRef item);

    // check if this entity can interact with the tile
    virtual InteractionAffordance can_interact(
        InteractionKind kind,
        const ivec3 &pos,
        Direction::Enum face,
        ItemRef item) const;

    // tries to interact this entity with tile at (pos, face)
    virtual InteractionResult interact(
        InteractionKind kind,
        const ivec3 &pos,
        Direction::Enum face,
        ItemRef item);

    // returns AABB if it entity has one
    virtual std::optional<AABB> aabb() const { return std::nullopt; }

    // returns true if gravity applies to this entity
    virtual bool has_gravity() const { return true; }

    // returns true if collisions with this entity are registered with other
    // entities
    virtual bool collides_with_entities() const { return true; }

    // returns true if collisions need to be checked in order to spawn this
    // entity
    virtual bool check_spawn_collision() const { return true; }

    // returns true if this entity should be allowed to travel vertically out
    // of the level boundaries but stay within a chunk
    virtual bool allow_vertically_out_of_level() const { return false; }

    // returns true if this entity causes a collision event with other
    virtual bool does_collide(const Entity &other) const { return true; }

    // returns true if this entity stops the movement of other
    virtual bool does_stop(const Entity &other) const { return true; }

    // called when this entity collides with another
    virtual void on_collision(Entity &other) {}

    // get lights for this entity
    virtual LightArray lights() const;

    // returns true if this entity should be highlighted by the cursor
    virtual bool highlight() const { return false; }
};
