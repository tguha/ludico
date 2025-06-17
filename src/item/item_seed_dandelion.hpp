#pragma once

#include "item/item_seed.hpp"
#include "item/item_basic_icon.hpp"
#include "locale.hpp"

struct ItemSeedDandelion
    : public ItemSeed,
      public ItemBasicIcon {
    using Base = ItemSeed;

    explicit ItemSeedDandelion(ItemId id)
        : Item(id),
          ItemSeed(id),
          ItemBasicIcon(id) {}

    std::string name() const override {
        return "seed_dandelion";
    }

    std::string locale_name() const override {
        return Locale::instance()["item:seed_dandelion"];
    }

    TextureArea sprite() const override;

    EntitySpawnFn spawn_fn(
        const ItemStack &item,
        const Entity &entity,
        const ivec3 &pos,
        Direction::Enum face) const override;

    InteractionAffordance can_plant(
        const ItemStack &item,
        const Entity &entity,
        const ivec3 &pos,
        Direction::Enum face) const override;
};
