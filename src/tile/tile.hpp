#pragma once

#include "util/types.hpp"
#include "util/util.hpp"
#include "util/assert.hpp"
#include "util/direction.hpp"
#include "util/type_registry.hpp"
#include "util/aabb.hpp"
#include "tile/tile_util.hpp"

struct TextureArea;
struct Level;
struct Entity;
struct TileRenderer;
struct ItemRef;
struct Item;
struct DropTable;
struct InteractionAffordance;
struct InteractionResult;
enum class InteractionKind : u8;

template <usize N>
struct NLightArray;
using LightArray = NLightArray<4>;

struct Tile {
    enum class Transparency {
        OFF = 0,
        ON,
        MERGED
    };

    TileId id;

    explicit Tile(TileId id) : id(id) {}
    virtual ~Tile() = default;

    inline operator TileId() const {
        return this->id;
    }

    virtual std::string name() const {
        return "(ERROR)";
    }

    virtual std::string locale_name() const {
        return "(ERROR)";
    }

    // item for this tile, nullptr if one does not exist
    virtual const Item *item() const {
        return nullptr;
    }

    // called at rate defined in level.hpp
    virtual void random_tick(
        Level &level,
        const ivec3 &pos) const {}

    // used to enable light gathering for this tile
    virtual bool can_emit_light() const {
        return false;
    }

    // tile lights, only used if can_emit_light() is true
    virtual LightArray lights(
        const Level &level,
        const ivec3 &pos) const;

    // if true, this tile can be subtiled
    virtual bool subtile() const {
        return false;
    }

    // base subtile value for tile when initially added, only used when
    // subtile() is true
    virtual u8 subtile_default(
        const Level &level,
        const ivec3 &pos) const {
        return 0xFF;
    }

    virtual std::optional<AABB> aabb(
        const Level &level,
        const ivec3 &pos) const {
        return AABB::unit().translate(vec3(pos));
    }

    virtual bool solid(
        const Level &level,
        const ivec3 &pos) const {
        return true;
    }

    virtual bool collides(
        const Level &level,
        const ivec3 &pos,
        const Entity &entity) const {
        return true;
    }

    // if true, tile can be destroyed
    virtual bool destructible(
        const Level &level,
        const ivec3 &pos) const {
        return true;
    }

    virtual DropTable drops(
        const Level &level,
        const ivec3 &pos) const;

    virtual TextureArea texture(
        const Level *level,
        const ivec3 &pos,
        Direction::Enum dir) const;

    // returns if this entity can interact with
    virtual InteractionAffordance can_interact_with_this(
        const Level &level,
        const ivec3 &pos,
        Direction::Enum face,
        InteractionKind kind,
        const Entity &e,
        ItemRef item) const;

    // tries to interact entity with this tile
    virtual InteractionResult interacted(
        Level &level,
        const ivec3 &pos,
        Direction::Enum face,
        InteractionKind kind,
        Entity &e,
        ItemRef item) const;

    virtual const TileRenderer &renderer() const = 0;

    virtual Transparency transparency_type() const {
        return Transparency::OFF;
    }

    std::string to_string() const {
        return fmt::format(
            "Tile({}, {}, {})",
            this->id,
            this->name(),
            this->locale_name());
    }
};

struct Tiles {
    static Tiles &get();

    void init();

    template <typename T>
    void register_type() {
        TileId id;

        if constexpr (requires () { T::PRESET_ID; }) {
            LOG(
                "using preset tile id {} for tile {}",
                T::PRESET_ID,
                typeid(T).name());
            id = T::PRESET_ID;
        } else {
            id = this->next_id();
        }

        ASSERT(
            !this->tiles[id].get(),
            "while registering {}: tile with id {} already present",
            typeid(T).name(),
            id);
        this->tiles[id] = std::make_unique<T>(id);
        this->by_index[std::type_index(typeid(T))] = this->tiles[id].get();
    }

    template <typename T>
    inline const T &get() const {
        const auto type_index = std::type_index(typeid(T));
        ASSERT(
            this->by_index.contains(type_index),
            "no such tile {}", std::string_view(type_index.name()));
        return dynamic_cast<T&>(*this->by_index.at(type_index));
    }

    inline const Tile &get(std::type_index type_index) const {
        ASSERT(
            this->by_index.contains(type_index),
            "no such tile {}", std::string_view(type_index.name()));
        return *this->by_index.at(type_index);
    }

    inline const Tile &operator[](TileId id) const {
        return *this->tiles.at(id);
    }

private:
    inline TileId next_id() {
        while (this->tiles[this->_next_id] && this->_next_id < MAX_TILES) {
            this->_next_id++;
        }
        ASSERT(this->_next_id != MAX_TILES, "ran out of tile IDs");
        return this->_next_id;
    }

    TileId _next_id = 1; // skip TileAir
    std::array<std::unique_ptr<Tile>, MAX_TILES> tiles;
    std::unordered_map<std::type_index, Tile*> by_index;
};

using TileRegistry = TypeRegistry<Tiles, Tile>;

template <typename T>
using TileRegistrar = TypeRegistrar<TileRegistry, T>;

#define DECL_TILE_REGISTER(_T)                                                 \
    const auto CONCAT(_T, __COUNTER__) = (TileRegistrar<_T>(), 1);
