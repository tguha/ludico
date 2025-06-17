#pragma once

#include "util/types.hpp"
#include "util/util.hpp"
#include "util/math.hpp"
#include "util/aabb.hpp"
#include "util/assert.hpp"
#include "util/direction.hpp"

struct VoxelShape {
    using ReshapeFn = std::function<void(VoxelShape&)>;

    uvec3 size;
    std::shared_ptr<bool[]> data;

    VoxelShape() = default;
    explicit VoxelShape(const uvec3 &size)
        : size(size),
          data(
              std::shared_ptr<bool[]>(new bool[size.x * size.y * size.z] {})) {}
    VoxelShape(const uvec3 &size, std::shared_ptr<bool[]> data)
        : size(size),
          data(data) {}

    VoxelShape deep_copy() const {
        auto res = VoxelShape(this->size);
        std::memcpy(
            &res.data[0],
            &data[0],
            size.x * size.y * size.z * sizeof(data[0]));
        return res;
    }

    inline bool present() const {
        return this->data.get() != nullptr;
    }

    inline bool contains(const auto &i) const {
        return i.x >= 0
            && i.y >= 0
            && i.z >= 0
            && i.x < decltype(i[0])(size.x)
            && i.y < decltype(i[1])(size.y)
            && i.z < decltype(i[2])(size.z);
    }

    inline auto &operator[](const auto &i) const {
        ASSERT(this->contains(i), "{}", i);
        return this->data[
            i.x * size.y * size.z + i.y * size.z + i.z];
    }

    inline auto &operator[](const auto &i) {
        ASSERT(this->contains(i), "{}", i);
        return this->data[
            i.x * size.y * size.z + i.y * size.z + i.z];
    }

    inline void clear() {
        ASSERT(this->present());
        std::memset(
            &data[0], 0, size.x * size.y * size.z * sizeof(data[0]));
    }

    inline AABBu bounds() const {
        ASSERT(this->present());
        AABBu result(size, uvec3(0));
        for (usize x = 0; x < size.x; x++) {
            for (usize y = 0; y < size.y; y++) {
                for (usize z = 0; z < size.z; z++) {
                    const auto p = uvec3(x, y, z);
                    if (!(*this)[p]) {
                        continue;
                    }

                    result.min = math::min(result.min, p);
                    result.max = math::max(result.max, p);
                }
            }
        }
        return AABBu::compute(result.min, result.max);
    }

    template <typename F>
    void each(F f) const {
        for (usize x = 0; x < size.x; x++) {
            for (usize y = 0; y < size.y; y++) {
                for (usize z = 0; z < size.z; z++) {
                    f({ x, y, z });
                }
            }
        }
    }

    inline void resize(const uvec3 &new_size) {
        auto new_shape = VoxelShape(new_size);

        new_shape.each(
            [&](const ivec3 &p) {
                if (this->contains(p) && new_shape.contains(p)) {
                    new_shape[p] = (*this)[p];
                }
            });

        *this = new_shape;
    }

    static VoxelShape merge(std::span<VoxelShape> shapes);

    // reshape to remove all corners from a voxel shape
    static void reshape_remove_corners(VoxelShape &shape);
};

