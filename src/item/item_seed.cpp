#include "item/item_seed.hpp"
#include "entity/entity.hpp"
#include "entity/entity_mob.hpp"
#include "entity/interaction.hpp"
#include "level/level.hpp"

InteractionAffordance ItemSeed::can_interact(
    InteractionKind kind,
    const ItemStack &item,
    const Entity &entity,
    const ivec3 &pos,
    Direction::Enum face) const {
    if (kind != InteractionKind::SECONDARY) {
        return InteractionAffordance::NONE;
    }
    return this->can_plant(item, entity, pos, face);
}

InteractionResult ItemSeed::interact(
    InteractionKind kind,
    const ItemStack &item,
    Entity &entity,
    const ivec3 &pos,
    Direction::Enum face) const {
    const auto spawn_fn =
        this->spawn_fn(
            item,
            entity,
            pos,
            face);
    const auto target = pos + Direction::to_ivec3(face);
    const auto success =
        spawn_fn(
            *entity.level,
            Entity::COLLISION_OFFSET
                + math::xyz_to_xnz(
                    Level::to_tile_center(pos),
                    f32(target.y)));

    if (const auto mob = EntityRef(entity).as<EntityMob>()) {
        if (auto *inventory = mob->inventory()) {
            inventory->remove(item, 1);
        }
    }

    return success ? InteractionResult::SUCCESS : InteractionResult::FAILURE;
}
