#pragma once

#include "util/types.hpp"
#include "util/util.hpp"
#include "util/resource.hpp"
#include "util/assert.hpp"
#include "util/moveable.hpp"

#include "gfx/bgfx.hpp"
#include "gfx/util.hpp"
#include "gfx/mesh_buffer.hpp"
#include "gfx/vertex.hpp"

struct Program;

struct Mesh {
    RDResource<bgfx::VertexBufferHandle> vertex_buffer;
    RDResource<bgfx::IndexBufferHandle> index_buffer;

    Mesh() = default;

    template<typename V, typename I>
    Mesh(const MeshBuffer<V, I> &buffer) {
        this->upload(
            V::layout,
            std::span(std::as_const(buffer.vertices)),
            std::span(std::as_const(buffer.indices)));
    }

    template<typename V, typename I>
    Mesh(
        const bgfx::VertexLayout &layout,
        std::span<const V> vertices,
        std::span<const I> indices = std::span<const I, 0>()) {
        this->upload(layout, vertices, indices);
    }

    template<typename V, typename I>
    void upload(
        const bgfx::VertexLayout &layout,
        std::span<const V> vertices,
        std::span<const I> indices = std::span<const I, 0>()) {
        this->vertex_buffer =
            RDResource<bgfx::VertexBufferHandle>(
                bgfx::createVertexBuffer(
                    bgfx::copy(&vertices[0], vertices.size() * sizeof(V)),
                    layout),
                [](auto &h) { bgfx::destroy(h); });

        // TODO: can we just render without indices?
        // generate indices if none are provided
        std::vector<I> is;
        if (indices.size() == 0) {
            is.reserve(vertices.size());

            for (I i = 0; i < vertices.size(); i++) {
                is.push_back(i);
            }

            indices = std::span(is);
        }

        u64 flags = BGFX_BUFFER_NONE;

        if constexpr (std::is_same_v<I, u32>) {
            flags |= BGFX_BUFFER_INDEX32;
        } else {
            static_assert(std::is_same_v<I, u16>);
        }

        this->index_buffer =
            RDResource<bgfx::IndexBufferHandle>(
                bgfx::createIndexBuffer(
                    bgfx::copy(&indices[0], indices.size() * sizeof(I)),
                    flags),
                [](auto &h) { bgfx::destroy(h); });
    }

    void render(
        const Program &program,
        const mat4 &model,
        RenderState render_state) const;
};

// extern template instantiations
#define _MESH_EXTERN_TEMPLATE(_V, _I, _E)  \
    _E template Mesh::Mesh(                \
        const MeshBuffer<_V, _I>&);        \
    _E template void Mesh::upload(         \
        const bgfx::VertexLayout &,        \
        std::span<const _V>,               \
        std::span<const _I>);

#define _MESH_EXTERN_TEMPLATE_I_E(_I, _E)                      \
    _MESH_EXTERN_TEMPLATE(VertexPosition,              _I, _E) \
    _MESH_EXTERN_TEMPLATE(VertexTexture,               _I, _E) \
    _MESH_EXTERN_TEMPLATE(VertexColor,                 _I, _E) \
    _MESH_EXTERN_TEMPLATE(VertexColorNormal,           _I, _E) \
    _MESH_EXTERN_TEMPLATE(VertexTextureNormal,         _I, _E) \
    _MESH_EXTERN_TEMPLATE(VertexTextureSpecularNormal, _I, _E) \
    _MESH_EXTERN_TEMPLATE(VertexTextureColor,          _I, _E)

_MESH_EXTERN_TEMPLATE_I_E(unsigned short, extern)
_MESH_EXTERN_TEMPLATE_I_E(unsigned int, extern)
