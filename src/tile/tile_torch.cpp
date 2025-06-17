#include "tile/tile_torch.hpp"
#include "gfx/specular_textures.hpp"
#include "gfx/sprite_resource.hpp"

// TODO: unify with item spritesheet somehow?
static const auto sprites =
    SpritesheetResource("tile_torch", "item/torch.png", uvec2(8, 8));

TileTorch::TileTorch(TileId id)
    : Tile(id) {
    auto model =
        VoxelModel::from_sprites(
            *sprites,
            {
                sprites[uvec2(1, 0)],
                sprites[uvec2(1, 0)],
                sprites[uvec2(1, 0)],
                sprites[uvec2(1, 0)],
                sprites[uvec2(2, 0)],
                sprites[uvec2(3, 0)]
            },
            Direction::expand_directional_array<
                std::array<std::optional<TextureArea>, Direction::COUNT>>(
                    SpecularTextures::instance().min()),
            {},
            VoxelModel::FLAG_MIN_BOUNDS);
    const auto offset_model =
        math::xyz_to_xnz(
            ((vec3(SCALE) - vec3(model.bounds.size())) / 2.0f)
                / vec3(SCALE), 0.0f);

    this->_renderer =
        TileRendererVoxelBasic(
            *this,
            std::move(model),
            offset_model);
    this->aabb_base =
        model.bounds
            .scale_min(1.0f / SCALE)
            .translate(offset_model);
}
