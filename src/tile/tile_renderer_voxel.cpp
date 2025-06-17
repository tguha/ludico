#include "tile/tile_renderer_voxel.hpp"
#include "tile/tile.hpp"
#include "constants.hpp"

void TileRendererVoxel::mesh(
    const Level *level,
    const ivec3 &pos,
    MeshBuffer<ChunkVertex, u32> &dest) const {
    const auto &tile = *this->tile;
    auto &model = this->get_model(level, pos);
    const auto offset = dest.num_vertices();

    const auto pos_base = this->offset() + vec3(pos);

    // preallocate
    usize i_v = dest.vertices.size();
    dest.vertices.resize(dest.vertices.size() + model.mesh.num_vertices());

    for (const auto &v : model.mesh.vertices) {
        ChunkVertex cv;
        cv.pos = pos_base + (v.position / vec3(SCALE));
        cv.st = v.st;
        cv.st_specular = v.st_specular;
        cv.normal = v.normal;
        cv.tile_id = tile.id;
        dest.vertices[i_v++] = cv;
    }

    usize i_i = dest.indices.size();
    dest.indices.resize(dest.indices.size() + model.mesh.num_indices());
    for (const auto &i : model.mesh.indices) {
        dest.indices[i_i++] = offset + i;
    }
}
