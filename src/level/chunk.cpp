#include "level/chunk.hpp"
#include "level/level.hpp"
#include "tile/tile_renderer.hpp"
#include "entity/entity.hpp"
#include "gfx/program.hpp"
#include "util/rand.hpp"
#include "util/macros.hpp"
#include "util/hash.hpp"
#include "util/time.hpp"
#include "util/types.hpp"
#include "serialize/serialize.hpp"
#include "global.hpp"

SERIALIZE_ENABLE()
DECL_SERIALIZER(Chunk)
DECL_PRIMITIVE_SERIALIZER(alloc_ptr<Chunk>)
DECL_PRIMITIVE_SERIALIZER(decltype(Chunk::data))

// traverse an area of this chunk with function F
template <typename F>
static void f_area(const AABBi &bounds, F &&f) {
    using ArgType =
        std::decay_t<
            typename types::unwrap_function<F>::template arg<0>::type>;
    for (isize x = bounds.min.x; x <= bounds.max.x; x++) {
        for (isize y = bounds.min.y; y <= bounds.max.y; y++) {
            for (isize z = bounds.min.z; z <= bounds.max.z; z++) {
                if constexpr (std::same_as<ArgType, ivec3>) {
                    f(ivec3(x, y, z));
                } else if constexpr (std::same_as<ArgType, Chunk::Offset>) {
                    f(Chunk::Offset(x, y, z));
                } else {
                    []<bool flag = false>() {
                        static_assert(flag, "invalid type");
                    }();
                }
            }
        }
    }
}

static void init(Chunk &chunk) {
    chunk.raw = decltype(Chunk::raw)(&chunk);
    chunk.tiles = decltype(Chunk::tiles)(&chunk);
    chunk.light = decltype(Chunk::light)(&chunk);
    chunk.subtile = decltype(Chunk::subtile)(&chunk);
    chunk.ghost = decltype(Chunk::ghost)(&chunk);
    chunk.flags = decltype(Chunk::flags)(&chunk);
}

Chunk::Chunk(Level &level, ivec2 offset)
    : level(&level),
      offset(offset),
      offset_tiles(ivec3(offset.x, 0, offset.y) * SIZE) {
    init(*this);
    std::memset(&this->data, 0, sizeof(this->data));
}

void Chunk::on_after_serialize(SerializationContext &ctx, Archive &a) {
    // write entities count, list
    Serializer<usize>().serialize(this->entities.size(), a, ctx);

    const auto s = Serializer<alloc_ptr<Entity>>();
    for (auto *ptr : this->entities) {
        // get alloc_ptr off of level
        s.serialize(
            this->level->entity_ptr(ptr->id),
            a,
            ctx);
    }
}

void Chunk::on_after_deserialize(SerializationContext &ctx, Archive &a) {
    // read entities count, list
    usize n_entities;
    Serializer<usize>().deserialize(n_entities, a, ctx);

    const auto s = Serializer<alloc_ptr<Entity>>();
    for (usize i = 0; i < n_entities; i++) {
        // read into alloc_ptr and spawn into level (will get added into chunk
        // this way)
        alloc_ptr<Entity> ptr;
        s.deserialize(ptr, a, ctx);
        this->level->load(std::move(ptr));
    }
}

void Chunk::on_resolve(SerializationContext &ctx) {
    init(*this);
}

void Chunk::update() {
    for (auto *e : this->entities) {
        e->update();
    }
}

void Chunk::tick() {
    auto rand = Rand(hash(this->offset, global.time->ticks));

    // number of random ticks in entire chunk per tick
    const auto n =
        (VOLUME * Level::RANDOM_TICKS_PER_MINUTE) /
            (TICKS_PER_SECOND * 60);

    for (usize i = 0; i < n; i++) {
        const auto pos = ivec3(
            rand.next(0, Chunk::SIZE.x - 1),
            rand.next(0, Chunk::SIZE.y - 1),
            rand.next(0, Chunk::SIZE.z - 1));

        const auto proxy = this->raw[pos];
        const TileId tile = Chunk::TileData::from(proxy);

        if (tile == 0) {
            continue;
        }

        Tiles::get()[tile].random_tick(
            *this->level,
            this->offset_tiles + pos);
    }

    for (auto *e : this->entities) {
        e->tick();
    }
}

std::tuple<usize, bool> Chunk::get_entities(
    std::span<Entity*> dest,
    const ivec2 &tile,
    std::optional<EntityFilterFn> filter) const {
    usize n = 0;
    const auto &xze = this->xz_entities[tile.x * Chunk::SIZE.z + tile.y];

    for (auto *e : xze) {
        if (filter && !(*filter)(*e)) {
            continue;
        }

        if (n >= dest.size()) {
            return std::make_tuple(n, true);
        }
        dest[n++] = e;
    }

    return std::make_tuple(n, false);
}

usize Chunk::num_entities(
    const AABB2i &bounds,
    std::optional<EntityFilterFn> filter) const {
    usize n = 0;
    for (auto x = bounds.min.x; x <= bounds.max.x; x++) {
        for (auto z = bounds.min.y; z <= bounds.max.y; z++) {
            n += this->num_entities(ivec2(x, z), filter);
        }
    }
    return n;
}

std::tuple<usize, bool> Chunk::get_entities(
    std::span<Entity*> dest,
    const AABB2i &bounds,
    std::optional<EntityFilterFn> filter) const {
    usize n = 0;

    for (auto x = bounds.min.x; x <= bounds.max.x; x++) {
        for (auto z = bounds.min.y; z <= bounds.max.y; z++) {
            const auto [m, overflow] =
                this->get_entities(dest.subspan(n), ivec2(x, z), filter);
            n += m;

            if (overflow) {
                return std::make_tuple(n, true);
            }
        }
    }

    return std::make_tuple(n, false);
}

std::tuple<usize, bool> Chunk::lights(
    std::span<Light> dest,
    const AABB2i &bounds) const {
    usize n = 0;

    // tiles
    bool overflow = false;
    f_area(
        AABBi(
            math::xz_to_xyz(bounds.min, 0),
            math::xz_to_xyz(bounds.max, Chunk::SIZE.y - 1)),
        [&](const ivec3 &pos) {
            if (overflow) {
                return;
            }

            const auto proxy = this->raw[pos];
            const auto tile_id = Chunk::TileData::from(proxy);

            if (tile_id == 0) {
                return;
            }

            const auto flags = Chunk::FlagsData::from(proxy);
            if (!(flags & TF_LIGHT)) {
                return;
            }

            const auto &tile = Tiles::get()[tile_id];
            const auto lights =
                tile.lights(
                    *this->level, this->offset_tiles + pos);

            for (const auto &l : lights) {
                if (n >= dest.size()) {
                    overflow = true;
                    return;
                }

                dest[n++] = l;
            }
        });

    if (overflow) {
        return std::make_tuple(n, true);
    }

    // get entity lights
    auto entities =
        global.frame_allocator.alloc_span<Entity*>(
            this->num_entities(bounds),
            Allocator::F_CALLOC);

    this->get_entities(entities, bounds);

    for (const auto *e : entities) {
        for (const auto &l : e->lights()) {
            if (n >= dest.size()) {
                return std::make_tuple(n, true);
            }

            dest[n++] = l;
        }
    }

    return std::make_tuple(n, false);
}

std::tuple<usize, bool> Chunk::offsets_with_flags(
    std::span<ivec3> dest,
    const AABB2i &bounds,
    Chunk::FlagsType flags) const {
    usize n = 0;
    bool overflow = false;
    f_area(
        AABBi(
            math::xz_to_xyz(bounds.min, 0),
            math::xz_to_xyz(bounds.max, Chunk::SIZE.y - 1)),
        [&](const Offset &offset) {
            if (overflow || (this->flags[offset] & flags) != flags) {
                return;
            }

            if (n >= dest.size()) {
                overflow = true;
                return;
            }

            dest[n++] = static_cast<ivec3>(offset);
        });

    return std::make_tuple(n, overflow);
}

Chunk::RawData::SafeProxy Chunk::or_level(const ivec3 &pos) {
    return Chunk::in_bounds(pos) ?
        this->raw.safe(pos)
        : this->level->raw[this->offset_tiles + pos];
}

Chunk *Chunk::neighbor(Direction::Enum d) {
    return this->level->chunkp(this->offset + Direction::to_ivec3(d).xz());
}

std::array<Chunk*, 4> Chunk::neighbors() {
    std::array<Chunk*, 4> res;

    const auto directions = {
        Direction::NORTH,
        Direction::EAST,
        Direction::SOUTH,
        Direction::WEST,
    };

    for (const auto d: directions) {
        res[d] = this->neighbor(d);
    }

    return res;
}

void Chunk::on_modify(
    DataType type, const ivec3 &pos, Data old_data, Data &new_data) {
    if (type == DT_TILE) {
        const auto
            old_tile = Chunk::TileData::from(old_data),
            new_tile = Chunk::TileData::from(new_data);
        const auto &tile = Tiles::get()[new_tile];
        const auto same_tile = new_tile == old_tile;

        // set subtile if applicable
        if (new_tile != 0 && tile.subtile()) {
            this->subtile[pos] =
                tile.subtile_default(
                    *this->level,
                    pos + this->offset_tiles);
        }

        // set tile flags
        u8 flags = 0;
        if (new_tile != 0) {
            flags |= tile.can_emit_light() ? TF_LIGHT : 0;
            flags |= tile.renderer().has_extras() ? TF_RENDER_EXTRAS : 0;
        }
        this->flags[pos] = flags;

        if (!same_tile) {
            const auto transparent =
                new_tile == 0
                    || tile.transparency_type() != Tile::Transparency::OFF;

            if (transparent) {
                Light::update(
                    *this->level,
                    this->offset_tiles + pos);
            } else if (Chunk::LightData::from(old_data) != 0) {
                // remove light if tile was replaced with solid
                Light::remove(
                    *this->level,
                    this->offset_tiles + pos,
                    Chunk::LightData::from(old_data));
            }

            // TODO: propagate if tile is light emitting
        }
    }
}
