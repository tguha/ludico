#pragma once

#include "tile/tile.hpp"
#include "locale.hpp"

struct TileGrass : public Tile {
    using Base = Tile;
    using Base::Base;

    std::string name() const override {
        return "grass";
    }

    std::string locale_name() const override {
        return Locale::instance()["tile:grass"];
    }

    DropTable drops(
        const Level &level,
        const ivec3 &pos) const override;

    const TileRenderer &renderer() const override;
};
