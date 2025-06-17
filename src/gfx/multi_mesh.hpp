#pragma once

#include "util/collections.hpp"
#include "util/tracker.hpp"
#include "util/types.hpp"
#include "util/util.hpp"
#include "util/resource.hpp"
#include "util/allocator.hpp"
#include "util/list.hpp"
#include "gfx/bgfx.hpp"
#include "gfx/util.hpp"
#include "gfx/mesh_buffer.hpp"
#include "gfx/vertex.hpp"

struct Program;
struct MultiMesh;
struct MultiMeshEntry;

namespace multi_mesh {
    using Id = u16;

    void render(
        const MultiMesh &base,
        usize index_offset,
        usize index_count,
        usize vertex_offset,
        usize vertex_count,
        const Program &program,
        const mat4 &model,
        RenderState render_state);

    // adds an entry to a mesh base, returns info about it
    MultiMeshEntry add(
        MultiMesh &base,
        usize n_vertices,
        std::span<const u8> vertices,
        std::span<const u16> indices);
}

// default fields for all subtypes of TMultiMesh
struct MultiMesh {
    // next ID to be assigned to an entry when add()-ed
    multi_mesh::Id next_entry_id = 0;

    // vertex layout
    const bgfx::VertexLayout *layout;

    // GPU buffer handles
    RDResource<bgfx::DynamicIndexBufferHandle> index_buffer;
    RDResource<bgfx::DynamicVertexBufferHandle> vertex_buffer;

    // total number of vertices and indices
    usize n_vertices = 0, n_indices = 0;

    // vertex data as bytes
    List<u8> vertex_data;

    // index data
    List<u16> index_data;

    // number of indices in the largest entry in the mesh
    usize n_indices_largest_entry = 0;

    MultiMesh() = default;
    MultiMesh(const MultiMesh&) = delete;
    MultiMesh(MultiMesh&&) = default;
    MultiMesh &operator=(const MultiMesh&) = delete;
    MultiMesh &operator=(MultiMesh&&) = default;
    virtual ~MultiMesh() = default;

    explicit MultiMesh(Allocator *allocator)
        : vertex_data(allocator),
          index_data(allocator) {}

    // number of entries
    virtual usize num_entries() const = 0;

    // gets the nth entry from this mesh
    virtual MultiMeshEntry &nth_entry(usize i) const = 0;

    // get generic entries from subtype
    virtual MultiMeshEntry &entry(multi_mesh::Id id) const = 0;
};

// defaults fields for all subtypes of TMultiMeshEntry
struct MultiMeshEntry {
    MultiMesh *mesh;
    multi_mesh::Id id;
    usize index_offset, index_count, vertex_offset, vertex_count;

    void render(
        const Program &program,
        const mat4 &model,
        RenderState render_state) const {
        multi_mesh::render(
            this->generic_parent(),
            this->index_offset,
            this->index_count,
            this->vertex_offset,
            this->vertex_count,
            program,
            model,
            render_state);
    }

    MultiMesh &generic_parent() const {
        return *this->mesh;
    }
};

template <typename M>
struct TMultiMeshEntry : public MultiMeshEntry {
    inline M &parent() const {
        return static_cast<M&>(*this->_parent);
    }
};

// "mesh" containing data for multiple meshes
template <typename V, typename Entry>
    requires std::is_default_constructible_v<Entry>
struct TMultiMesh : public MultiMesh {
    using Base = MultiMesh;

    BlockList<Entry, 8> entries;

    explicit TMultiMesh(
        Allocator *allocator,
        usize size_indices = 256,
        usize size_vertices = 256)
        : Base(allocator),
          entries(allocator) {
        this->layout = &V::layout;
        this->index_buffer =
            gfx::as_bgfx_resource(
                bgfx::createDynamicIndexBuffer(
                    size_indices,
                    BGFX_BUFFER_ALLOW_RESIZE));
        this->vertex_buffer =
            gfx::as_bgfx_resource(
                bgfx::createDynamicVertexBuffer(
                    size_vertices,
                    *this->layout,
                    BGFX_BUFFER_ALLOW_RESIZE));
    }

    TMultiMesh(TMultiMesh &&other) { *this = std::move(other); }

    TMultiMesh &operator=(TMultiMesh &&other) {
        Base::operator=(std::move(other));
        this->entries = std::move(other.entries);

        for (auto &e : this->entries) {
            e.mesh = this;
        }

        return *this;
    }

    usize num_entries() const override {
        return this->entries.size();
    }

    MultiMeshEntry &nth_entry(usize i) const override {
        return const_cast<MultiMeshEntry&>(
            static_cast<const MultiMeshEntry&>(
                this->entries[i]));
    }

    MultiMeshEntry &entry(
        multi_mesh::Id id) const override {
        return (*this)[id];
    }

    inline Entry &operator[](multi_mesh::Id id) const {
        return const_cast<Entry&>(this->entries[id]);
    }

protected:
    Entry &add(const MeshBuffer<V> &buffer) {
        return this->add(
            std::span(std::as_const(buffer.vertices)),
            std::span(std::as_const(buffer.indices)));
    }

    template <size_t N, size_t M>
    Entry &add(
        std::span<const V, N> vertices,
        std::span<const u16, M> indices) {
        return this->add(
            vertices.size(),
            collections::bytes(vertices),
            indices.subspan(0));
    }

    Entry &add(
        usize n_vertices,
        std::span<const u8, std::dynamic_extent> vertices,
        std::span<const u16, std::dynamic_extent> indices) {
        ASSERT(this);
        Entry e;
        auto result =
            multi_mesh::add(
                *this,
                n_vertices,
                vertices,
                indices);
        static_cast<MultiMeshEntry&>(e) = result;
        return this->entries.emplace(std::move(e));
    }
};
