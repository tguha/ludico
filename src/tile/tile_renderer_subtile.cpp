#include "tile/tile_renderer_subtile.hpp"
#include "level/level.hpp"

void TileRendererSubtile::mesh(
    const Level *level,
    const ivec3 &pos,
    MeshBuffer<ChunkVertex, u32> &dst) const {
    TileRenderer::mesh_subtile(
        this->tile,
        level,
        pos,
        dst,
        // TODO: better subtile support
        level ? level->subtile[pos] : 0xFF);
}
