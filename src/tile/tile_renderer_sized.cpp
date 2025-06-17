#include "tile/tile_renderer_sized.hpp"
#include "tile/tile.hpp"
#include "gfx/cube.hpp"

void TileRendererSized::mesh(
    const Level *level,
    const ivec3 &pos,
    MeshBuffer<ChunkVertex, u32> &dst) const {
    const auto size = this->size();
    const auto offset_pos = (vec3(1.0f) - size) / 2.0f;

    for (const auto &d : Direction::ALL) {
        const auto offset = dst.num_vertices();
        auto
            area_tex = this->coords(level, pos, d),
            area_specular = this->coords_specular(level, pos, d);

        area_tex.min += area_tex.texel / 2.0f;
        area_tex.max -= area_tex.texel / 2.0f;

        const auto
            area_tex_size = area_tex.max - area_tex.min,
            area_specular_size = area_specular.max - area_specular.min;

        for (usize i = 0; i < 4; i++) {
            const auto p =
                cube::VERTICES[
                    cube::INDICES[
                        (usize(d) * 6) + cube::UNIQUE_INDICES[i]]];
            ChunkVertex vertex;
            vertex.pos = vec3(pos) + (p * size) + offset_pos;
            vertex.normal = cube::NORMALS[d];
            vertex.st =
                (cube::TEX_COORDS[i] * area_tex_size)
                    + area_tex.min;
            vertex.st_specular =
                (cube::TEX_COORDS[i] * area_specular_size)
                    + area_specular.min;
            vertex.tile_id = this->tile->id;
            dst.vertices.push_back(vertex);
        }

        for (usize i : cube::FACE_INDICES) {
            dst.indices.push_back(offset + i);
        }
    }
}
