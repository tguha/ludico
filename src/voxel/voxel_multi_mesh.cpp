#include "voxel/voxel_multi_mesh.hpp"

const VoxelMultiMeshEntry &VoxelMultiMesh::add(VoxelModel &&model) {
    auto &entry = Base::add(model.mesh);
    entry.model = std::move(model);
    return entry;
}
