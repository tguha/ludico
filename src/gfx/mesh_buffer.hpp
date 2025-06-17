#pragma once

#include "util/types.hpp"

template<typename V, typename I = u16>
struct MeshBuffer {
    using VertexType = V;
    using IndexType = I;

    std::vector<V> vertices;
    std::vector<I> indices;

    MeshBuffer(const MeshBuffer&) = default;
    MeshBuffer(MeshBuffer&&) = default;
    MeshBuffer &operator=(const MeshBuffer&) = delete;
    MeshBuffer &operator=(MeshBuffer&&) = default;

    MeshBuffer(usize vertices_size = 16, usize indices_size = 32) {
        this->vertices.reserve(vertices_size);
        this->indices.reserve(indices_size);
    }

    inline void append(const MeshBuffer<V, I> &other) {
        this->vertices.insert(
            this->vertices.end(), other.vertices.begin(), other.vertices.end());
        this->indices.insert(
            this->indices.end(), other.indices.begin(), other.indices.end());
    }

    inline void reset() {
        this->vertices.clear();
        this->indices.clear();
    }

    inline usize num_vertices() const {
        return this->vertices.size();
    }

    inline usize num_indices() const {
        return this->indices.size();
    }
};

template <typename T>
struct is_mesh_buffer: public std::false_type {};

template <typename V, typename I>
struct is_mesh_buffer<MeshBuffer<V, I>> : public std::true_type {};
