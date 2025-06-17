#pragma once

#include "tile/tile.hpp"
#include "item/item_tile.hpp"
#include "locale.hpp"

struct ItemStone;

struct TileStone : public Tile {
    using Base = Tile;
    using Base::Base;

    std::string name() const override {
        return "stone";
    }

    std::string locale_name() const override {
        return Locale::instance()["tile:stone"];
    }

    DropTable drops(
        const Level &level,
        const ivec3 &pos) const override;

    const Item *item() const override;

    const TileRenderer &renderer() const override;
};

struct ItemStone : public ItemTile {
    explicit ItemStone(ItemId id)
        : ItemTile(id, Tiles::get().get<TileStone>()) {}
};
