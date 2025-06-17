#include "levelgen/gen.hpp"
#include "level/level.hpp"
#include "level/subtile.hpp"
#include "tile/tile_grass.hpp"
#include "tile/tile_water.hpp"
#include "util/noise.hpp"
#include "util/rand.hpp"
#include "util/managed_resource.hpp"
#include "wfc/wfc.hpp"
#include "tile/tile_stone.hpp"
#include "tile/tile_dirt.hpp"
#include "platform/platform.hpp"
#include "gfx/image.hpp"
#include "global.hpp"

static const auto pattern_image =
   ValueManagedResource<Image>(
        []() {
            return Image(
                gfx::load_texture_data(
                    global.platform->resources_path +
                        "/gen/gen_default.png",
                    PixelFormat::RGBA).unwrap());
        });

void DefaultLevelGenerator::generate(Level &level) {
    using WFCType = wfc::WFC<u32, 2, 3, 300>;
    auto wfc = WFCType(
        ivec2(pattern_image->size),
        &pattern_image[0],
        wfc::PatternFunction::WEIGHTED,
        wfc::NextCellFunction::MIN_ENTROPY,
        wfc::BorderBehavior::CLAMP,
        Rand(0x12345667),
        wfc::FLAG_REFLECT | wfc::FLAG_ROTATE);

    Image result(uvec2(32, 32));
    wfc.collapse(
        ivec2(result.size),
        &result[0]);

    const auto outline = result.scaled(uvec2(level.size * Chunk::SIZE.xz()));
	const auto aabb_l = level.aabb_tile();
    const auto center_l = aabb_l.center();
    auto rand = Rand(123455);

    for (isize x = 0; x < aabb_l.max.x; x++) {
        for (isize z = 0; z < aabb_l.max.y; z++) {
            const auto d =
                math::length(
                    (vec2(x, z) - vec2(center_l))
                        / (vec2(aabb_l.size()) / 2.0f));

            if (d + rand.next(0.0f, 0.05f) < 0.3f) {
			    level.tiles[{ x, 0, z }] = Tiles::get().get<TileDirt>();
			    level.tiles[{ x, 1, z }] = Tiles::get().get<TileGrass>();
            }
        }
    }
}
