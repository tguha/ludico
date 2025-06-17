#include "tile/tile_renderer_basic.hpp"
#include "tile/tile.hpp"
#include "gfx/cube.hpp"

void TileRendererBasic::mesh(
    const Level *level,
    const ivec3 &pos,
    MeshBuffer<ChunkVertex, u32> &dst) const {
    for (const auto &d : Direction::ALL) {
        const auto offset = dst.num_vertices();
        const auto
            area_tex = this->coords(level, pos, d),
            area_specular = this->coords_specular(level, pos, d);

        for (usize i = 0; i < 4; i++) {
            const auto p =
                cube::VERTICES[
                    cube::INDICES[
                        (usize(d) * 6) + cube::UNIQUE_INDICES[i]]];
            ChunkVertex vertex;
            vertex.pos = vec3(pos) + p;
            vertex.normal = cube::NORMALS[d];
            vertex.st =
                (cube::TEX_COORDS[i] * area_tex.sprite_unit)
                    + area_tex.min;
            vertex.st_specular =
                (cube::TEX_COORDS[i] * area_specular.sprite_unit)
                    + area_specular.min;
            vertex.tile_id = this->tile->id;
            dst.vertices.push_back(vertex);
        }

        for (usize i : cube::FACE_INDICES) {
            dst.indices.push_back(offset + i);
        }
    }
}
