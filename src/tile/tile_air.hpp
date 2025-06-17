#pragma once

#include "tile/tile.hpp"

struct TileAir : public Tile {
    using Base = Tile;
    using Base::Base;

    static constexpr auto PRESET_ID = 0;

    std::string name() const override {
        return "air";
    }

    std::string locale_name() const override {
        return "BUG :(";
    }

    bool solid(const Level&, const ivec3&) const override {
        return false;
    }

    bool destructible(const Level&, const ivec3&) const override {
        return false;
    }

    bool collides(
        const Level&,
        const ivec3&,
        const Entity&) const override {
        return false;
    }

    Transparency transparency_type() const override {
        return Transparency::ON;
    }

    const TileRenderer &renderer() const override;
};
