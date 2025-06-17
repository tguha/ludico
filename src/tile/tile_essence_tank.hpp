#pragma once

#include "tile/tile.hpp"
#include "item/item_tile.hpp"
#include "locale.hpp"

struct TileEssenceTank : public Tile {
    using Base = Tile;
    using Base::Base;

    std::string name() const override {
        return "essence_tank";
    }

    std::string locale_name() const override {
        return Locale::instance()["tile:essence_tank"];
    }

    Transparency transparency_type() const override {
        return Transparency::ON;
    }

    DropTable drops(
        const Level &level,
        const ivec3 &pos) const override;

    const Item *item() const override;

    const TileRenderer &renderer() const override;
};

struct ItemEssenceTank : public ItemTile {
    explicit ItemEssenceTank(ItemId id)
        : ItemTile(id, Tiles::get().get<TileEssenceTank>()) {}
};
