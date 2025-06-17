#pragma once

#include "tile/tile.hpp"
#include "item/item_tile.hpp"
#include "locale.hpp"

struct TileDirt : public Tile {
    using Base = Tile;
    using Base::Base;

    std::string name() const override {
        return "dirt";
    }

    std::string locale_name() const override {
        return Locale::instance()["tile:dirt"];
    }

    DropTable drops(
        const Level &level,
        const ivec3 &pos) const override;

    const Item *item() const override;

    const TileRenderer &renderer() const override;
};

struct ItemDirt : public ItemTile {
    explicit ItemDirt(ItemId id)
        : ItemTile(id, Tiles::get().get<TileDirt>()) {}
};
