#pragma once

#include "item/item.hpp"

struct ItemSeed : virtual public Item {
    using Base = Item;
    using Base::Base;

    // returns a function which tries to spawn an entity corresponding to this
    // seed type
    virtual EntitySpawnFn spawn_fn(
        const ItemStack &item,
        const Entity &entity,
        const ivec3 &pos,
        Direction::Enum face) const = 0;

    // returns true if this seed can be planted at the specified location
    virtual InteractionAffordance can_plant(
        const ItemStack &item,
        const Entity &entity,
        const ivec3 &pos,
        Direction::Enum face) const = 0;

    usize stack_size() const override {
        return 99;
    }

    InteractionAffordance can_interact(
        InteractionKind kind,
        const ItemStack &item,
        const Entity &entity,
        const ivec3 &pos,
        Direction::Enum face) const override;

    InteractionResult interact(
        InteractionKind kind,
        const ItemStack &item,
        Entity &entity,
        const ivec3 &pos,
        Direction::Enum face) const override;
};
