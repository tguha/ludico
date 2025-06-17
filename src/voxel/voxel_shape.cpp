#include "voxel/voxel_shape.hpp"

VoxelShape VoxelShape::merge(std::span<VoxelShape> shapes) {
    ASSERT(shapes.size() != 0);

    auto size = shapes[0].size;
    for (const auto &s : shapes) {
        size = math::max(size, s.size);
    }

    auto result = VoxelShape(size);
    for (const auto &s : shapes) {
        s.each([&](const ivec3 &p) { result[p] |= s[p]; });
    }

    return result;
}

void VoxelShape::reshape_remove_corners(VoxelShape &shape) {
    const auto copy = shape.deep_copy();

    for (usize x = 0; x < shape.size.x; x++) {
        for (usize y = 0; y < shape.size.y; y++) {
            for (usize z = 0; z < shape.size.z; z++) {
                const auto p = ivec3(x, y, z);
                if (!shape[p]) {
                    continue;
                }

                usize neighbors = 0;
                for (const auto &d : Direction::ALL) {
                    const auto n = p + Direction::to_ivec3(d);
                    if (copy.contains(n) && copy[n]) {
                        neighbors++;
                    }
                }

                if (neighbors <= 3) {
                    shape[p] = false;
                }
            }
        }
    }
}


