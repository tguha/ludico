#pragma once

#include "util/util.hpp"
#include "util/types.hpp"
#include "util/math.hpp"
#include "util/direction.hpp"
#include "gfx/vertex.hpp"
#include "gfx/mesh_buffer.hpp"
#include "voxel/voxel_shape.hpp"

struct TextureAtlasEntry;
struct MultiTexture;
struct TextureArea;

struct VoxelModel {
    using VertexType = VertexTextureSpecularNormal;

    enum Flags {
        FLAG_NONE           = 0,
        FLAG_MIN_BOUNDS     = 1 << 0,
        FLAG_DISCARD_SHAPE  = 1 << 1
    };

    // voxel palette as texture atlas entry
    // currently always 16x16 (256) color
    struct VoxelPalette {
        static constexpr auto SIZE = uvec2(16, 16);
        const TextureAtlasEntry *entry;
    };

    // f(face direction, pos a, pos b) -> true if direction faces of a and b can
    // be merged
    using CanMergeFn =
        std::function<bool(Direction::Enum, const uvec3&, const uvec3&)>;

    // f(face direction, start pos 3D, end pos 3D (inclusive), size 2D) -> area
    using TexcoordsFn =
        std::function<
            AABB2(Direction::Enum, const uvec3&, const uvec3&, const uvec2&)>;

    // inclusive bounds
    AABB bounds;

    // shape data
    VoxelShape shape;

    // mesh buffer
    MeshBuffer<VertexType> mesh;

    // offsets for each texture coordinate axis
    std::array<vec2, Direction::COUNT> st_offsets;

    inline VoxelModel deep_copy() const {
        return VoxelModel {
            this->bounds,
            this->shape.deep_copy(),
            this->mesh,
            this->st_offsets
        };
    }

    static VoxelModel merge(std::span<VoxelModel> models);

    // create a voxel model in the specified shape using the specified merging/
    // texture coordinate lookup functions
    static VoxelModel make(
        const VoxelShape &shape,
        CanMergeFn &&can_merge_fn,
        TexcoordsFn &&texcoords_fn,
        TexcoordsFn &&specular_fn,
        usize options = 0);

    // create a voxel model with vertices stored at the specified destination
    // texture coordinates are resolved first using 'texcoords', otherwise (if the
    // coordinates are not specified in some direction), the texture coordinates
    // specified for the face in 'resolve_faces' is used and the texture
    // coordinates are clamped to that of the face which they are resolved to.
    // uses the 3d shape stored in 'shape' to determine which voxels are present
    // within the level of 'size'
    static VoxelModel make(
        const VoxelShape &shape,
        const vec2 &st_unit,
        const std::array<std::optional<AABB2>, Direction::COUNT> &texcoords,
        const std::array<std::optional<AABB2>, Direction::COUNT> &speccoords,
        const std::array<Direction::Enum, Direction::COUNT> &resolve_faces,
        usize options = 0);

    // create voxel model according to specified sprite coordinates, otherwise
    // using specified resolve faces
    static VoxelModel from_sprites(
        const MultiTexture &texture,
        const std::array<std::optional<TextureArea>, Direction::COUNT>
            &areas_tex,
        const std::array<std::optional<TextureArea>, Direction::COUNT>
            &areas_spec,
        const std::array<Direction::Enum, Direction::COUNT> &resolve,
        usize options = 0,
        std::optional<VoxelShape::ReshapeFn> reshape = std::nullopt);

    // create voxel model according to specified sprite coordinates, otherwise
    // using specified resolve faces
    static VoxelModel
        from_sprites_mirrored(
            const MultiTexture &texture,
            const std::array<std::optional<TextureArea>, 3> &areas_tex,
            const std::array<std::optional<TextureArea>, 3> &areas_spec,
            const std::array<Direction::Enum, 3> &resolve,
            usize options = 0,
            std::optional<VoxelShape::ReshapeFn> reshape = std::nullopt);

    static VoxelModel from_sprite(
        const MultiTexture &texture,
        const TextureArea &area_tex,
        const TextureArea &area_spec,
        int depth,
        usize options = 0,
        std::optional<VoxelShape::ReshapeFn> reshape = std::nullopt);

    // load palette from .vox file
    static VoxelPalette load_palette(const std::string &path);

    // load from .vox file
    static VoxelModel load(
        const std::string &path,
        const VoxelPalette &palette);
};
