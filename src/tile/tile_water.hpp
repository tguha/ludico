#pragma once

#include "tile/tile.hpp"
#include "item/item_tile.hpp"
#include "locale.hpp"

struct ItemWater;

struct TileWater : public Tile {
    using Base = Tile;
    using Base::Base;

    std::string name() const override {
        return "water";
    }

    std::string locale_name() const override {
        return Locale::instance()["tile:water"];
    }

    DropTable drops(
        const Level &level,
        const ivec3 &pos) const override;

    const Item *item() const override;

    const TileRenderer &renderer() const override;
};

struct ItemWater : public ItemTile {
    explicit ItemWater(ItemId id)
        : ItemTile(id, Tiles::get().get<TileWater>()) {}
};
