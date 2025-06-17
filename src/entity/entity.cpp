#include "entity/entity.hpp"
#include "entity/interaction.hpp"
#include "serialize/context_level.hpp"
#include "serialize/serialize.hpp"
#include "util/time.hpp"
#include "level/light.hpp"
#include "global.hpp"

SERIALIZE_ENABLE()
DECL_SERIALIZER(Entity)

static constexpr auto
    RESOLUTION_EPSILON = 0.0005f,
    MOVEMENT_EPSILON = 0.00001f;

static void remove_from_current_chunk(Entity &entity) {
    if (!entity.chunk) {
        return;
    }

    entity.chunk->get_xz_entities(Level::to_chunk_pos(entity.last_tile).xz())
        .erase(&entity);
    entity.chunk->entities.erase(&entity);
}

// run whenever entity position is changed
static void check_pos_change(Entity &entity, bool create = false) {
    if (!create
        && math::all(
            math::epsilonEqual(
                entity.pos, entity.last_pos, math::epsilon<f32>()))) {
        return;
    }

    entity.last_move_ticks = global.time->ticks;

    // re-compute positions
    entity.center = entity.pos;

    if (const auto box = entity.aabb()) {
        entity.center = box->center();
    }

    entity.tile = Level::to_tile(entity.pos);
    entity.center_tile = Level::to_tile(entity.center);
}

// run whenever entity tile is changed
static void check_tile_change(Entity &entity, bool create = false) {
    if (!create && entity.tile == entity.last_tile) {
        return;
    }

    const auto offset = Level::to_offset(entity.tile);

    if (!entity.level->contains_chunk(offset)) {
        WARN("Entity {} moved out of level, disappearing!", entity.id);
        entity.destroy();
        return;
    }

    if (!entity.chunk ||
            offset != entity.chunk->offset) {
        auto *chunk = entity.level->chunkp(offset);
        remove_from_current_chunk(entity);
        entity.chunk = chunk;
        entity.chunk->entities.insert(&entity);
        entity.chunk->get_xz_entities(
            Level::to_chunk_pos(entity.tile).xz()).insert(&entity);
    } else {
        // move xz entities in current chunk
        entity.chunk->get_xz_entities(
            Level::to_chunk_pos(entity.last_tile).xz()).erase(&entity);
        entity.chunk->get_xz_entities(
            Level::to_chunk_pos(entity.tile).xz()).insert(&entity);
    }

    entity.last_tile = entity.tile;
}

void Entity::on_resolve(SerializationContext &ctx) {
    this->on_level_change();
}

void Entity::init() {
    this->spawned_tick = global.time->ticks;
}

void Entity::on_level_change() {
    check_pos_change(*this, true);
    check_tile_change(*this, true);
}

void Entity::tick() {
    // move according to velocity
    const auto moved = this->move(this->velocity);

    for (usize i = 0; i < 3; i++) {
        if (math::abs(moved[i]) < math::abs(this->velocity[i]) - 0.001f) {
            this->velocity[i] *= this->restitution ? -(*this->restitution) : 0.0f;
        }
    }

    if (math::length(this->velocity) < 0.00001f) {
        this->velocity = vec3(0);
    } else if (
        !math::isnan(this->drag) && this->drag >= math::epsilon<f32>()) {
        this->velocity *= 1.0f / (this->drag + 0.01f);
    }

    if (this->has_gravity()) {
        this->velocity += GRAVITY;
    }

    if (math::length(this->pos_delta) > 0.001f) {
        this->direction = math::normalize(this->pos_delta);
    }

    this->direction.tick();
}

void Entity::relocate() {
    this->pos_delta = this->pos - this->last_pos;
    check_pos_change(*this);
    check_tile_change(*this);
    this->last_pos = pos;
}

// removes entity from its current containing chunk/level
void Entity::detach() {
    this->level = nullptr;
    remove_from_current_chunk(*this);
}

bool Entity::grounded() const {
    return this->level->collides(
        math::xyz_to_xnz(this->center, this->pos.y)
            + vec3(0.0f, -0.01f, 0.0f),
        this,
        std::nullopt,
        Level::COLLIDER_TYPE_TILE);
}

// returns legal movement for the AABB on the specified axis
static std::tuple<f32, Entity*> move_axis(
    Entity &entity,
    AABB box,
    f32 movement,
    std::span<AABB> colliders,
    std::span<Entity*> entities,
    const vec3 &axis) {
    if (math::abs(movement) < MOVEMENT_EPSILON) {
        return std::make_tuple(0.0f, nullptr);
    }

    auto d_v = axis * movement;
    const auto sign = math::sign(movement);
    const auto index = math::nonzero_axis(axis);

    auto moved = box.translate(d_v);

    // entity which collides with this entity and stops it
    Entity *collision_entity = nullptr;

    for (usize i = 0; i < colliders.size(); i++) {
        const auto &c = colliders[i];
        if (!c.collides(moved)) {
            continue;
        }

        if (entities.size() > 0 && entities[i]) {
            auto &other = *entities[i];

            if (!entity.does_collide(other)
                    || !other.does_collide(entity)) {
                continue;
            }

            // emit collision events
            entity.on_collision(other);
            other.on_collision(entity);

            // entities can collide (emit collision events) but not stop each
            // other
            if (!entity.does_stop(other)
                    || !other.does_stop(entity)) {
                continue;
            }

            collision_entity = &other;
        }

        // compute collision depth, resolve, and re-translate
        const auto depth = moved.depth(c)[index];
        d_v[index] += -sign * (depth + RESOLUTION_EPSILON);
        moved = box.translate(d_v);

        // zero movement if it was effectively stopped by this collision
        if (math::abs(d_v[index]) < MOVEMENT_EPSILON) {
            d_v[index] = 0.0f;
        }
    }

    const auto result = d_v[index];
    return std::make_tuple(
        math::abs(result) < math::epsilon<f32>() ? 0.0f : result,
        collision_entity);
}


vec3 Entity::move(const vec3 &movement) {
    if (math::length(movement) < MOVEMENT_EPSILON) {
        return vec3(0);
    }

    const auto aabb_opt = this->aabb();

    if (!aabb_opt) {
        this->pos += movement;
        return movement;
    }

    auto aabb_current = *aabb_opt;
    const auto aabb_start = aabb_current;
    const auto aabb_level = this->level->aabb();

    // get colliders
    std::span<AABB> colliders;
    std::span<Entity*> entities;

    auto bounds = Level::aabb_to_tile(aabb_start.scale_center(4.0f));
    usize n_colliders;
    bool overflow;

    if (!this->collides_with_entities()) {
        colliders = global.tick_allocator.alloc_span<AABB>(128);

        const auto [n_c, of] =
            this->level->tile_colliders(
                colliders,
                bounds,
                this);
        n_colliders = n_c;
        overflow = of;
    } else {
        const auto n_c_allocated = this->level->num_entities(bounds) + 128;
        colliders = global.tick_allocator.alloc_span<AABB>(n_c_allocated);
        entities =
            global.tick_allocator.alloc_span<Entity*>(
                n_c_allocated,
                Allocator::F_CALLOC);

        const auto [n_c, of] =
            this->level->colliders(
                std::span(colliders),
                bounds,
                this,
                entities,
                [](const Entity &e) { return e.collides_with_entities(); });

        n_colliders = n_c;
        overflow = of;
    }

    if (overflow) {
        WARN("Entity colliders overflow for {}", this->id);
    }

    // move along each axis
    for (usize i = 0; i < 3; i++) {
        const auto axis = math::make_axis<vec3>(i);
        const auto [movement_axis, _] =
            move_axis(
                *this,
                aabb_current,
                movement[i],
                colliders.subspan(0, n_colliders),
                entities.size() == 0 ?
                    std::span<Entity*>()
                    : entities.subspan(0, n_colliders),
                axis);

        // do not allow movement if it would move entity out of bounds
        const auto aabb_new = aabb_current.translate(axis * movement_axis);

        bool inside = true;
        if (i != math::AXIS_Y
                || !this->allow_vertically_out_of_level()) {
            for (const auto &p : aabb_new.points()) {
                if (!aabb_level.contains(p)) {
                    inside = false;
                    break;
                }
            }
        }

        if (inside) {
            aabb_current = aabb_new;
        }
    }

    const auto d = aabb_current.min - aabb_start.min;
    this->pos += d;
    return d;
}

void Entity::on_pickup(const ItemStack &stack, EntityRef item) {

}

InteractionAffordance Entity::can_interact_with_this(
    InteractionKind kind, const Entity &other, ItemRef item) const {
    return InteractionAffordance::NONE;
}

InteractionAffordance Entity::can_interact(
    InteractionKind kind,
    const Entity &target,
    ItemRef item) const {
    return InteractionAffordance::NONE;
}

InteractionResult Entity::interacted(
    InteractionKind kind,
    Entity &other,
    ItemRef item) {
    return InteractionResult::NONE;
}

InteractionResult Entity::interact(
    InteractionKind kind,
    Entity &target,
    ItemRef item) {
    return InteractionResult::NONE;
}

InteractionAffordance Entity::can_interact(
    InteractionKind kind,
    const ivec3 &pos,
    Direction::Enum face,
    ItemRef item) const {
    return InteractionAffordance::NONE;
}

InteractionResult Entity::interact(
    InteractionKind kind,
    const ivec3 &pos,
    Direction::Enum face,
    ItemRef item) {
    return InteractionResult::NONE;
}

LightArray Entity::lights() const { return LightArray(); }
