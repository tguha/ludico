#pragma once

#include "item/item.hpp"
#include "item/item_icon.hpp"

struct Tile;

struct ItemTile : public Item {
    ItemTile(ItemId id, const Tile &tile);

    inline const Tile &tile() const {
        return *this->_tile;
    }

    std::string name() const override;

    std::string locale_name() const override;

    usize stack_size() const override {
        return 99;
    }

    HoldType hold_type() const override {
        return HoldType::TILE;
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

    const ItemIcon &icon() const override {
        return this->_icon;
    }

    void render_held(
        RenderContext &ctx,
        const ItemStack &item,
        const Entity &entity,
        EntityRef target,
        const ivec3 &pos,
        Direction::Enum face) const override;

private:
    const Tile *_tile;
    ItemIconTile _icon;
};
