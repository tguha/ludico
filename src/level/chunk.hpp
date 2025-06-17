#pragma once

#include "util/types.hpp"
#include "util/util.hpp"
#include "util/math.hpp"
#include "util/resource.hpp"
#include "util/direction.hpp"
#include "serialize/annotations.hpp"
#include "level/light.hpp"
#include "tile/tile.hpp"
#include "entity/util.hpp"

struct Level;
struct Entity;
struct Archive;
struct SerializationContext;

struct Chunk {
    static constexpr ivec3 SIZE = ivec3(32, 8, 32);
    static constexpr usize VOLUME = SIZE.x * SIZE.y * SIZE.z;

    // compressed offset into chunk data (u16 sized)
    struct Offset {
        Offset(const ivec3 &pos) : Offset(pos.x, pos.y, pos.z) {
            ASSERT(Chunk::in_bounds(pos));
        }

        Offset(const uvec3 &pos) : Offset(pos.x, pos.y, pos.z) {}

        Offset(u16 x, u16 y, u16 z)
            : i(((x & 0x1F) << 8)
                | ((y & 0x7) << 5)
                | ((z & 0x1F) << 0)) {}

        inline operator ivec3() const {
            return ivec3(static_cast<uvec3>(*this));
        }

        inline operator uvec3() const {
            return uvec3(
                (i >> 8) & 0x1F,
                (i >> 5) & 0x7,
                (i >> 0) & 0x1F);
        }

        explicit operator u16() const {
            return i;
        }
private:
    u16 i = 0;
    } PACKED;

    // base flag type
    using FlagsType = u8;

    // FlagsData flags
    enum TileFlags : FlagsType {
        TF_LIGHT = (1 << 0),            // if tile.can_emit_light()
        TF_RENDER_EXTRAS = (1 << 1),    // if tile.renderer().has_extras()
        TF_COUNT = sizeof(FlagsType) * 8 // total number of possible flags
    };

    // chunk data base type
    using Data = u64;

    enum DataType {
        DT_RAW, DT_TILE, DT_LIGHT, DT_SUBTILE, DT_GHOST, DT_FLAGS
    };

    enum DataTypeFlags {
        DTF_NONE,
        DTF_ON_MODIFY = 1 << 0,
        DTF_BUMP_RENDER = 1 << 1
    };

    static constexpr usize DATA_TYPE_FLAGS[] = {
        [DT_RAW] = DTF_ON_MODIFY | DTF_BUMP_RENDER,
        [DT_TILE] = DTF_ON_MODIFY | DTF_BUMP_RENDER,
        [DT_LIGHT] = DTF_NONE,
        [DT_SUBTILE] = DTF_ON_MODIFY | DTF_BUMP_RENDER,
        [DT_GHOST] = DTF_BUMP_RENDER,
        [DT_FLAGS] = DTF_ON_MODIFY | DTF_BUMP_RENDER,
    };

    // proxy for access to chunk data
    template <typename T, usize O, usize M, DataType D>
    struct ChunkDataAccess final {
        using Type = ChunkDataAccess<T, O, M, D>;

        // allow access to template parameters
        using _T = T;
        static constexpr auto
            _O = O,
            _M = M;
        static constexpr auto
            _D = D;

        // proxy for access to individual element
        struct Proxy {
            using ParentType = Type;

            ChunkDataAccess *parent = nullptr;
            Data *data = nullptr;

            Proxy() = default;

            Proxy(ChunkDataAccess *parent, const Offset &o)
                : Proxy(parent, static_cast<u16>(o)) { }

            Proxy(ChunkDataAccess *parent, u16 i)
                : parent(parent),
                  data(&(parent->chunk->data[i])) { }

            Proxy(ChunkDataAccess *parent, Data *data)
                : parent(parent), data(data) { }

            inline Data *ptr() const {
                return this->data;
            }

            inline T get() const {
                return static_cast<T>(*this);
            }

            inline operator T() const {
                return static_cast<T>(((*this->data) & M) >> O);
            }

            inline Proxy &operator=(T value) {
                ChunkDataAccess<T, O, M, D>::set(*this, value);
                return *this;
            }
        };

        // zeroable proxy, a little slower than a Proxy due to the null check
        // will also update neighboring chunks on assignment
        struct SafeProxy {
            using ParentType = Type;

            ivec3 pos;
            Proxy p;

            SafeProxy(ChunkDataAccess *parent, Data *data)
                : p(parent, data) { }
            SafeProxy(ChunkDataAccess *parent, const ivec3 &pos) {
                this->pos = pos;
                this->p = Chunk::in_bounds(pos) ?
                    Proxy(parent, pos) : Proxy(nullptr, nullptr);
            }

            inline Data *ptr() const {
                return this->p.data;
            }

            inline bool present() const {
                return this->p.data;
            }

            inline T get() const {
                return static_cast<T>(*this);
            }

            inline operator T() const {
                return this->p.data ? static_cast<T>(p) : 0;
            }

            inline SafeProxy &operator=(T value) {
                ASSERT(this->present());
                ChunkDataAccess<T, O, M, D>::set(*this, value);
                return *this;
            }
        };

        // proxy which evaluates to zero and crashes on assignment
        static const inline auto ZERO = SafeProxy(nullptr, nullptr);

        Chunk *chunk;

        ChunkDataAccess() = default;
        explicit ChunkDataAccess(Chunk *chunk) : chunk(chunk) {}

        inline const Proxy operator[](usize index) const {
            return Proxy(const_cast<ChunkDataAccess*>(this), index);
        }

        inline const Proxy operator[](const Offset &o) const {
            return Proxy(const_cast<ChunkDataAccess*>(this), o);
        }

        inline Proxy operator[](usize index) {
            return Proxy(this, index);
        }

        inline auto operator[](const Offset &o) {
            return Proxy(this, o);
        }

        inline const SafeProxy safe(const ivec3 &p) const {
            return SafeProxy(const_cast<ChunkDataAccess*>(this), p);
        }

        inline const SafeProxy safe(const Offset &o) const {
            return SafeProxy(const_cast<ChunkDataAccess*>(this), o);
        }

        inline auto safe(const ivec3 &p) {
            return SafeProxy(this, p);
        }

        inline auto safe(const Offset &o) {
            return SafeProxy(this, o);
        }

        inline Proxy as_proxy(Data &d) {
            return Proxy(this, &d);
        }

        template <typename U>
        static inline T from(const U &d) {
            return static_cast<T>((static_cast<Data>(d) & M) >> O);
        }

        template <typename U>
        static inline void set(U &d, T v) {
            _set<U, false>(d, v);
        }

        template <typename U>
        static inline void set_no_mesh(U &d, T v) {
            _set<U, true>(d, v);
        }

private:
        template <
            typename U,
            bool DO_NOT_MESH,
            bool IS_SAFE =
                std::is_same_v<
                    typename U::ParentType::SafeProxy,
                    std::remove_const_t<U>>>
        static inline void _set(U &d, T v) {
            // data type
            constexpr auto dt = U::ParentType::_D;
            constexpr auto dtf = DATA_TYPE_FLAGS[dt];

            Data *p = d.ptr(), old = *p;
            *p = ((*p) & ~M) | ((static_cast<Data>(v) << O) & M);

            Chunk *c;
            if constexpr (!IS_SAFE) {
                c = d.parent->chunk;
            } else {
                c = d.p.parent->chunk;

                if constexpr (dtf & DTF_ON_MODIFY) {
                    d.p.parent->chunk->on_modify(dt, d.pos, old, *p);
                }
            }

            // bump version of this chunk
            c->version++;

            constexpr auto bump_render =
                !DO_NOT_MESH && (dtf & DTF_BUMP_RENDER);

            if constexpr (bump_render) {
                c->render_version++;
            }

            if constexpr (IS_SAFE) {
                // bump neighboring versions if value changed
                const auto border = Chunk::border(d.pos);
                if (border && v != d.get()) {
                    auto *neighbor = c->neighbor(*border);
                    if (neighbor) {
                        neighbor->version++;

                        if constexpr (bump_render) {
                            neighbor->render_version++;
                        }
                    }
                }
            }
        }
    };

    std::array<Chunk::Data, Chunk::VOLUME> data;

    [[SERIALIZE_BY_CTX(SERIALIZE_IGNORE, SerializationContextLevel::get_level)]]
    Level *level;

    ivec2 offset;
    ivec3 offset_tiles;

    // arbitrary integer, *must* change every time chunk data is updated
    u64 version;

    // version, but it only changes when the chunk could need to be remeshed
    u64 render_version;

    // data accessors
    // types are declared explicitly for easy use of their static methods
    using RawData =
        ChunkDataAccess<
            Data, 0, std::numeric_limits<Data>::max(), DT_RAW>;

    using TileData =
        ChunkDataAccess<TileId, 0, 0xFFFF, DT_TILE>;

    using LightData =
        ChunkDataAccess<u8, 16, 0xF0000, DT_LIGHT>;

    using SubtileData =
        ChunkDataAccess<u8, 28, 0xFF0000000, DT_SUBTILE>;

    using GhostData =
        ChunkDataAccess<bool, 55, 0x0080000000000000, DT_GHOST>;

    using FlagsData =
        ChunkDataAccess<FlagsType, 56, 0xFF00000000000000, DT_FLAGS>;

    [[SERIALIZE_IGNORE]] RawData raw;
    [[SERIALIZE_IGNORE]] TileData tiles;
    [[SERIALIZE_IGNORE]] LightData light;
    [[SERIALIZE_IGNORE]] SubtileData subtile;
    [[SERIALIZE_IGNORE]] GhostData ghost;
    [[SERIALIZE_IGNORE]] FlagsData flags;

    // tracked by entity, see entity.{hpp, cpp}
    [[SERIALIZE_IGNORE]]
    std::unordered_set<Entity*> entities;

    [[SERIALIZE_IGNORE]]
    std::array<std::unordered_set<Entity*>, Chunk::SIZE.x * Chunk::SIZE.z>
        xz_entities;

    Chunk(Level &level, ivec2 offset);

    Chunk() = default;
    Chunk(const Chunk &other) = delete;
    Chunk(Chunk &&other) = default;
    Chunk &operator=(const Chunk &other) = delete;
    Chunk &operator=(Chunk &&other) = default;

    void on_after_serialize(SerializationContext &ctx, Archive &a);

    void on_after_deserialize(SerializationContext &ctx, Archive &a);

    void on_resolve(SerializationContext &ctx);

    inline auto operator[](usize index) {
        return this->raw[index];
    }

    inline auto operator[](usize index) const {
        return this->raw[index];
    }

    inline auto operator[](const ivec3 &p) {
        return this->raw[p];
    }

    inline auto operator[](const ivec3 &p) const {
        return this->raw[p];
    }

    inline auto &get_xz_entities(const ivec2 &tile) {
        ASSERT(tile.x >= 0 && tile.y >= 0
               && tile.x < Chunk::SIZE.x && tile.y < Chunk::SIZE.z);
        return this->xz_entities[tile.x * Chunk::SIZE.z + tile.y];
    }

    inline auto &get_xz_entities(const ivec2 &tile) const {
        ASSERT(tile.x >= 0 && tile.y >= 0
               && tile.x < Chunk::SIZE.x && tile.y < Chunk::SIZE.z);
        return this->xz_entities[tile.x * Chunk::SIZE.z + tile.y];
    }

    // number of entities on a tile
    inline usize num_entities(
        const ivec2 &tile,
        std::optional<EntityFilterFn> filter = std::nullopt) const {
        if (!filter) {
            return this->get_xz_entities(tile).size();
        } else {
            const auto &xze = this->get_xz_entities(tile);
            return std::count_if(
                xze.begin(),
                xze.end(),
                [&](Entity *e) { return (*filter)(*e); });
        }
    }

    // retrieve entities on a tile
    std::tuple<usize, bool> get_entities(
        std::span<Entity*> dest,
        const ivec2 &tile,
        std::optional<EntityFilterFn> filter = std::nullopt) const;

    // number of entities in an area
    usize num_entities(
        const AABB2i &bounds,
        std::optional<EntityFilterFn> filter = std::nullopt) const;

    // retrieve entities in a specified area
    std::tuple<usize, bool> get_entities(
        std::span<Entity*> dest,
        const AABB2i &bounds,
        std::optional<EntityFilterFn> filter = std::nullopt) const;

    // retrieves lights in the chunk
    std::tuple<usize, bool> lights(
        std::span<Light> dest,
        const AABB2i &bounds) const;

    // gets tile offsets in specified area which match the specified flags
    std::tuple<usize, bool> offsets_with_flags(
        std::span<ivec3> dest,
        const AABB2i &bounds,
        Chunk::FlagsType flags) const;

    // retrieve neighbor in specified direction
    // returns nullptr if not present
    Chunk *neighbor(Direction::Enum d);

    // retrieves data at the specified position or gets it from this chunk's
    // level if out of bounds
    Chunk::RawData::SafeProxy or_level(const ivec3 &pos);

    // retrieve all chunk neighbors
    // array entry is nullptr if not present
    std::array<Chunk*, 4> neighbors();

    // called whenever chunk is modified via safe proxy AND DataType flag
    // DTF_ON_MODIFY is specified for "type"
    void on_modify(
        DataType type, const ivec3 &pos, Data old_data, Data &new_data);

    void update();
    void tick();

    // utility functions
    static inline bool in_bounds(const ivec3 &pos) {
        return pos.x >= 0
            && pos.y >= 0
            && pos.z >= 0
            && pos.x < SIZE.x
            && pos.y < SIZE.y
            && pos.z < SIZE.z;
    }

    // returns true if the specified position is on a chunk border
    static inline bool on_border(const ivec3 &pos) {
        return pos.x == 0
            || pos.y == 0
            || pos.z == 0
            || pos.x == SIZE.x - 1
            || pos.y == SIZE.y - 1
            || pos.z == SIZE.z - 1;
    }

    // gets the border of the specified postition, nullopt if not on border
    static inline std::optional<Direction::Enum> border(
        const ivec3 &pos) {
        if (pos.x == 0) {
            return std::make_optional(Direction::WEST);
        } else if (pos.y == 0) {
            return std::make_optional(Direction::BOTTOM);
        } else if (pos.z == 0) {
            return std::make_optional(Direction::NORTH);
        } else if (pos.x == SIZE.x - 1) {
            return std::make_optional(Direction::EAST);
        } else if (pos.y == SIZE.y - 1) {
            return std::make_optional(Direction::TOP);
        } else if (pos.z == SIZE.z - 1) {
            return std::make_optional(Direction::SOUTH);
        }

        return std::nullopt;
    }
};
