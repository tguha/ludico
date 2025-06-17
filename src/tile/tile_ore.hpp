#pragma once

#include "tile/tile.hpp"
#include "tile/tile_renderer_ore.hpp"
#include "locale.hpp"

struct TileOre : public Tile {
    using Base = Tile;
    using Base::Base;

    virtual TextureArea coords_tex() const = 0;

    virtual TextureArea coords_specular() const = 0;

    std::optional<AABB> aabb(
        const Level &level,
        const ivec3 &pos) const override {
        return AABB::unit()
            .translate(vec3(pos))
            .scale_center(vec3(0.3f, 0.8f, 0.3f));
    }

    Transparency transparency_type() const override {
        return Transparency::ON;
    }

    // TODO
    DropTable drops(const Level&, const ivec3&) const override;

    const TileRenderer &renderer() const override {
        if (!this->_renderer) {
            this->_renderer =
                TileRendererOre(
                    *this,
                    this->coords_tex(),
                    this->coords_specular(),
                    hash(this->name()));
        }

        return *this->_renderer;
    }

protected:
    mutable std::optional<TileRendererOre> _renderer;
};

struct TileOreCopper : public TileOre {
    using Base = TileOre;
    using Base::Base;

    std::string name() const override {
        return "copper_ore";
    }

    std::string locale_name() const override {
        return Locale::instance()["tile:copper"];
    }

    TextureArea coords_tex() const override;

    TextureArea coords_specular() const override;
};

struct TileOreIron : public TileOre {
    using Base = TileOre;
    using Base::Base;

    std::string name() const override {
        return "iron_ore";
    }

    std::string locale_name() const override {
        return Locale::instance()["tile:iron"];
    }

    TextureArea coords_tex() const override;

    TextureArea coords_specular() const override;
};

struct TileOreSilver : public TileOre {
    using Base = TileOre;
    using Base::Base;

    std::string name() const override {
        return "silver_ore";
    }

    std::string locale_name() const override {
        return Locale::instance()["tile:silver"];
    }

    TextureArea coords_tex() const override;

    TextureArea coords_specular() const override;
};

struct TileOreGold : public TileOre {
    using Base = TileOre;
    using Base::Base;

    std::string name() const override {
        return "gold_ore";
    }

    std::string locale_name() const override {
        return Locale::instance()["tile:gold"];
    }

    TextureArea coords_tex() const override;

    TextureArea coords_specular() const override;
};
