#pragma once

#include "util/util.hpp"
#include "util/types.hpp"
#include "util/math.hpp"
#include "util/aabb.hpp"
#include "util/optional.hpp"
#include "util/direction.hpp"
#include "util/macros.hpp"
#include "gfx/mesh_buffer.hpp"
#include "gfx/vertex.hpp"
#include "gfx/cube.hpp"

namespace gfx {
template <typename P>
concept Primitive = requires (const P &p) {
    { p.emit(std::declval<MeshBuffer<VertexPosition>&>()) }
        -> std::same_as<void>;
    { p.is_transparent() }
        -> std::same_as<bool>;
};

struct PrimitiveTri {
    std::array<vec3, 3> vertices;
    std::optional<std::array<vec3, 3>> st;
    std::optional<std::array<vec3, 3>> st_specular;
    std::optional<vec4> color;
    std::optional<vec3> normal;

    template <typename V, typename I>
    void emit(MeshBuffer<V, I> &dst) {
        const auto
            offset_v = dst.vertices.size(),
            offset_i = dst.indices.size();
        dst.vertices.resize(offset_v + 3);
        dst.vertices.resize(offset_i + 3);
        for (usize i = 0; i < 3; i++) {
            V v;
            v.position = this->vertices[i];

            if constexpr (VertexHasST<V>) {
                util::map(this->st, [&](auto &arr) { v.st = arr[i]; });
            }

            if constexpr (VertexHasSTSpecular<V>) {
                util::map(
                    this->st_specular,
                    [&](auto &arr) { v.st_specular = arr[i]; });
            }

            if constexpr (VertexHasNormal<V>) {
                util::map(this->normal, [&](auto &n) { v.normal = n[i]; });
            }

            if constexpr (VertexHasColor<V>) {
                util::map(this->color, [&](auto &c) { v.color = c; });
            }

            dst.vertices[offset_v + i] = v;
            dst.indices[offset_i + i] = offset_v + i;
        }
    }

    bool is_transparent() const {
        return this->color && this->color->a < 1.0f;
    }
};

struct PrimitiveBox {
    AABB bounds;
    std::optional<std::array<AABB2, 6>> texcoords;
    std::optional<std::array<AABB2, 6>> speccoords;
    std::optional<std::array<vec4, 6>> colors;

    PrimitiveBox(
        AABB bounds,
        std::optional<std::array<AABB2, 6>> texcoords,
        std::optional<std::array<AABB2, 6>> speccoords,
        std::optional<std::array<vec4, 6>> colors)
        : bounds(bounds),
          texcoords(texcoords),
          speccoords(speccoords),
          colors(colors) {}

    PrimitiveBox(
        const AABB &bounds,
        const vec4 &color)
        : bounds(bounds),
          colors({ color, color, color, color, color, color }) {}

    template <typename V, typename I>
    static inline void emit_face(
        MeshBuffer<V, I> &dst,
        AABB bounds,
        Direction::Enum face,
        std::optional<AABB2> texcoords = std::nullopt,
        std::optional<AABB2> speccoords = std::nullopt,
        std::optional<vec4> color = std::nullopt) {
        const auto
            offset_v = dst.vertices.size(),
            offset_i = dst.indices.size();
        dst.vertices.resize(dst.vertices.size() + 4);
        dst.indices.resize(dst.indices.size() + 6);

        for (usize i = 0; i < 4; i++) {
            const auto base =
                cube::VERTICES[cube::INDICES[(face * 6) +
                    cube::UNIQUE_INDICES[i]]];

            V v;
            v.position = bounds.min + (base * bounds.size());

            if constexpr (VertexHasST<V>) {
                util::map(
                    texcoords,
                    [&](auto &tc) {
                        v.st =
                            tc.min +
                                (cube::TEX_COORDS[i % cube::TEX_COORDS.size()]
                                    * tc.size());
                    });
            }

            if constexpr (VertexHasSTSpecular<V>) {
                util::map(
                    speccoords,
                    [&](auto &sc) {
                        v.st_specular =
                            sc.min +
                                (cube::TEX_COORDS[i % cube::TEX_COORDS.size()]
                                    * sc.size());
                    });
            }

            if constexpr (VertexHasNormal<V>) {
                v.normal = Direction::to_vec3(face);
            }

            if constexpr (VertexHasColor<V>) {
                util::map(color, [&](auto &c) { v.color = c; });
            }

            dst.vertices[offset_v + i] = v;
        }

        for (usize i = 0; i < cube::FACE_INDICES.size(); i++) {
            dst.indices[offset_i + i] = offset_v + cube::FACE_INDICES[i];
        }
    }

    template <typename V, typename I>
    void emit(MeshBuffer<V, I> &dst, usize cull = 0) const {
        for (const auto d: Direction::ALL) {
            if (cull & (1 << d)) {
                continue;
            }

            this->emit_face<V, I>(
                dst,
                bounds,
                d,
                util::map(this->texcoords, [&](auto &xs) { return xs[d]; }),
                util::map(this->speccoords, [&](auto &xs) { return xs[d]; }),
                util::map(this->colors, [&](auto &xs) { return xs[d]; }));
        }
    }

    bool is_transparent() const {
        return this->colors
            && std::any_of(
                this->colors->begin(),
                this->colors->end(),
                [](const vec4 &c) { return c.a < 1.0f; });
    }
};
}
