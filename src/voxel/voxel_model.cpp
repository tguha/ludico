#include "voxel/voxel_model.hpp"
#include "gfx/primitive.hpp"
#include "gfx/texture_atlas.hpp"
#include "global.hpp"

#define OGT_VOX_IMPLEMENTATION
#include "ext/ogt_vox.hpp"

struct Face {
    Direction::Enum dir;
    uvec3 pos;
    vec2 size;
    AABB2 st;
    AABB2 st_specular;

    Face(
        Direction::Enum dir,
        uvec3 pos,
        vec2 size,
        AABB2 st,
        AABB2 st_specular)
        : dir(dir),
          pos(pos),
          size(size),
          st(st),
          st_specular(st_specular) {}
};

// get the texcoords for the specified face d of pixel (trixel? voxel?) p
// if the tex coords are not found at texcoords[d], then
// texcoords[resolve_faces[d]] is used instead.
static std::tuple<vec2, vec2> face_texcoords(
    const std::array<std::optional<AABB2>, 6> &texcoords,
    const std::array<Direction::Enum, 6> &resolve_faces,
    const vec2 &st_unit,
    const uvec3 &p,
    Direction::Enum d) {

    const auto d_resolved = texcoords[d] ? d : resolve_faces[d];

    int
        a0 = Direction::axis(d_resolved),
        a1 = (a0 + 1) % 3,
        a2 = (a0 + 2) % 3;

    // flip s/t coordinates when not on the z axis for proper texture indexing
    if (a0 != math::AXIS_Z) {
        std::swap(a1, a2);
    }

    const auto
        st_base =
        texcoords[d_resolved]->min + st_unit * vec2(p[a1], p[a2]);
    return std::make_tuple(st_base, st_base + st_unit);
}

// emit face into vertex buffer
template <typename V, typename I>
static void emit_face(
    MeshBuffer<V, I> &dst,
    const Face &face) {

    // a0 is dominant axis (s), a1 is t and a2 is p
    const auto
        a0 = Direction::axis(face.dir),
        a1 = (a0 + 1) % 3,
        a2 = (a0 + 2) % 3;

    // expand face along (t, p) axes
    const auto s = math::from_axes<vec3>(
        {{ a0, 1.0f },
         { a1, face.size.x },
         { a2, face.size.y }});

    gfx::PrimitiveBox::emit_face<V, I>(
        dst,
        AABB::unit()
            .scale_min(s)
            .translate(vec3(face.pos)),
        face.dir,
        face.st,
        face.st_specular);
}

VoxelModel VoxelModel::make(
    const VoxelShape &shape,
    CanMergeFn &&can_merge_fn,
    TexcoordsFn &&texcoords_fn,
    TexcoordsFn &&speccoords_fn,
    usize options) {

    MeshBuffer<VertexType> dst;

    for (auto d = Direction::SOUTH;
         d < Direction::COUNT;
         d = Direction::Enum(d + 1)) {
        const ivec3 d_v = Direction::to_ivec3(d);

        // find axes (s, t, p)
        // TODO: consider reordering for cache friendliness
        const usize
            s = math::nonzero_axis(d_v),
            t = (s + 1) % 3,
            p = (s + 2) % 3;

        // current position
        uvec3 u(0);

        // for each slice on d
        for (u[s] = 0; u[s] < shape.size[s]; u[s]++) {
            // mask index, visible face count
            usize i = 0, c = 0;

            // compute visible faces
            // TODO: consider a better storage mechanism
            bool slice[shape.size[p] * shape.size[t]];
            std::memset(slice, 0, sizeof(slice));

            for (u[p] = 0; u[p] < shape.size[p]; u[p]++) {
                for (u[t] = 0; u[t] < shape.size[t]; u[t]++, i++) {
                    if (!shape[u]) {
                        continue;
                    }

                    // neighbor in direction d
                    const auto n = ivec3(u) + d_v;

                    bool visible =
                        !AABBi(ivec3(shape.size - uvec3(1))).contains(n)
                            || !shape[uvec3(n)];
                    c += visible;
                    slice[i] = visible;
                }
            }

            // nothing visible? skip meshing
            if (c == 0) {
                continue;
            }

            // reset mask index
            i = 0;

            // combine faces in this slice
            for (u[p] = 0; u[p] < shape.size[p]; u[p]++) {
                for (u[t] = 0; u[t] < shape.size[t];) {

                    // skip non-visible faces
                    if (!slice[i]) {
                        u[t]++;
                        i++;
                        continue;
                    }

                    // combined size, w in t and h in p
                    int w, h;

                    // expand as far as possible along w
                    for (w = 1; u[t] + w < shape.size[t] && slice[i + w]; w++) {
                        const auto r = u + math::make_axis<uvec3>(t, w);
                        if (!can_merge_fn(d, u, r)) {
                            break;
                        }
                    }

                    // expand as far as possible aong h
                    for (h = 1; u[p] + h < shape.size[p]; h++) {
                        // check along w
                        for (int k = 0; k < w; k++) {
                            if (!slice[(u[p] + h) * shape.size[t] + (u[t] + k)]) {
                                goto expand_done;
                            }

                            const auto r = u
                                + math::from_axes<uvec3>(
                                    {{ t, w }, { p, h }});

                            if (!can_merge_fn(d, u, r)) {
                                goto expand_done;
                            }
                        }
                    }

expand_done:
                    // v is u + (w, h)
                    const auto v =
                        u + math::from_axes<uvec3>({{ t, w }, { p, h }});

                    const auto
                        texcoords = texcoords_fn(d, u, v, { w, h }),
                        speccoords = speccoords_fn(d, u, v, { w, h });

                    // emit face
                    emit_face(
                        dst,
                        Face(
                            d, u,
                            { w, h },
                            texcoords,
                            speccoords));

                    // clear slice on the combined face
                    for (int b = 0; b < h; b++) {
                        for (int a = 0; a < w; a++) {
                            slice[((u[p] + b) * shape.size[t]) + (u[t] + a)] =
                                false;
                        }
                    }

                    u[t] += w;
                    i += w;
                }
            }
        }
    }

    // compute min/max positions
    auto min = dst.vertices[0].position,
         max = dst.vertices[0].position;

    for (const auto &v : dst.vertices) {
        min = math::min(min, v.position);
        max = math::max(max, v.position);
    }

    // minimize if shape should be min-bounded
    if (options & FLAG_MIN_BOUNDS) {
        for (auto &v : dst.vertices) {
            v.position -= min;
        }
    }

    auto shape_result = shape;

    const auto bounds =
        AABB(min, max).translate((options & FLAG_MIN_BOUNDS) ? -min : vec3(0));

    if (options & FLAG_DISCARD_SHAPE) {
        shape_result.data.reset();
    }

    // compute base offsets for textures from all sides
    std::array<vec2, Direction::COUNT> st_offsets;
    for (const auto &d : Direction::ALL) {
        st_offsets[d] = texcoords_fn(d, uvec3(0), uvec3(1), uvec2(1)).min;
    }

    return VoxelModel { bounds, shape_result, std::move(dst), st_offsets };
}

VoxelModel VoxelModel::make(
    const VoxelShape &shape,
    const vec2 &st_unit,
    const std::array<std::optional<AABB2>, 6> &texcoords,
    const std::array<std::optional<AABB2>, 6> &speccoords,
    const std::array<Direction::Enum, 6> &resolve_faces,
    usize options) {
    const auto make_texcoords_fn =
        [&](const auto &coords) {
            return [&](
                Direction::Enum d,
                const uvec3 &p,
                const uvec3 &v,
                const uvec2 &s) {
                // get min/max face texcoords
                const auto st_min = std::get<0>(
                    face_texcoords(
                        coords, resolve_faces,
                        st_unit, p, d));
                const auto st_max = std::get<0>(
                    face_texcoords(
                        coords, resolve_faces,
                        st_unit, v, d));
                return AABB2(st_min, st_max);
            };
        };

    return VoxelModel::make(
        shape,
        [&](Direction::Enum d, const uvec3 &p, const uvec3 &q) {
            return true;
        },
        make_texcoords_fn(texcoords),
        make_texcoords_fn(speccoords),
        options);
}

VoxelModel VoxelModel::from_sprites(
    const MultiTexture &texture,
    const std::array<std::optional<TextureArea>, 6> &areas_tex,
    const std::array<std::optional<TextureArea>, 6> &areas_spec,
    const std::array<Direction::Enum, 6> &resolve,
    usize options,
    std::optional<VoxelShape::ReshapeFn> reshape) {

    const auto sprite_size = texture.unit_size;
    std::array<std::vector<u8>, 6> pixels;

    for (usize i = 0; i < 6; i++) {
        if (areas_tex[i]) {
            pixels[i] = std::vector<u8>(sprite_size.x * sprite_size.y * 4);
            texture.pixels(std::span(pixels[i]), *areas_tex[i]);
        }
    }

    // generate shape
    auto shape = VoxelShape(uvec3(sprite_size.x));

    const auto is_pixel_present = [&](
        const std::vector<u8> &pixels,
        const ivec2 &index) {
        return pixels[(((index.y * sprite_size.x) + index.x) * 4) + 3] != 0;
    };

    for (uint x = 0; x < shape.size.x; x++) {
        for (uint y = 0; y < shape.size.y; y++) {
            for (uint z = 0; z < shape.size.z; z++) {
                const ivec2 pixel_indices[] = {
                    { x, y },
                    { z, y },
                    { x, z }
                };

                bool present = true;
                for (usize i = 0; i < 6; i++) {
                    if (!areas_tex[i]) {
                        continue;
                    }

                    present &=
                        is_pixel_present(pixels[i], pixel_indices[i / 2]);
                }

                shape[uvec3(x, y, z)] = present;
            }
        }
    }

    if (reshape) {
        (*reshape)(shape);
    }

    std::array<std::optional<AABB2>, 6> texcoords;
    std::array<std::optional<AABB2>, 6> speccoords;

    std::copy(areas_tex.begin(), areas_tex.end(), texcoords.begin());
    std::copy(areas_spec.begin(), areas_spec.end(), speccoords.begin());

    return make(
        shape,
        texture.texel,
        texcoords,
        speccoords,
        resolve,
        options);
}

VoxelModel VoxelModel::from_sprites_mirrored(
        const MultiTexture &texture,
        const std::array<std::optional<TextureArea>, 3> &areas_tex,
        const std::array<std::optional<TextureArea>, 3> &areas_spec,
        const std::array<Direction::Enum, 3> &resolve,
        usize options,
        std::optional<VoxelShape::ReshapeFn> reshape) {
    return from_sprites(
        texture,
        Direction::expand_directional_array<
            std::array<std::optional<TextureArea>, Direction::COUNT>>(
                areas_tex),
        Direction::expand_directional_array<
            std::array<std::optional<TextureArea>, Direction::COUNT>>(
                areas_spec),
        Direction::expand_directional_array<
            std::array<Direction::Enum, Direction::COUNT>>(
                resolve),
        options, reshape);
}

VoxelModel VoxelModel::from_sprite(
    const MultiTexture &texture,
    const TextureArea &area_tex,
    const TextureArea &area_spec,
    int depth,
    usize options,
    std::optional<VoxelShape::ReshapeFn> reshape) {
    // retrieve pixel data
    const auto size_xy = texture.unit_size;
    std::vector<u8> pixels(size_xy.x * size_xy.y * 4);
    texture.pixels(std::span(pixels), area_tex);

    // generate shape
    auto shape = VoxelShape(uvec3(size_xy, depth));

    for (usize x = 0; x < shape.size.x; x++) {
        for (usize y = 0; y < shape.size.y; y++) {
            const auto p = pixels[(((y * shape.size.x) + x) * 4) + 3] != 0;

            for (usize z = 0; z < shape.size.z; z++) {
                shape[uvec3(x, y, z)] = p;
            }
        }
    }

    if (reshape) {
        (*reshape)(shape);
    }

    return make(
        shape,
        texture.texel,
        {
            area_tex, area_tex,
            std::nullopt, std::nullopt, std::nullopt, std::nullopt
        },
        {
            area_spec, area_spec,
            std::nullopt, std::nullopt, std::nullopt, std::nullopt
        },
        {
            Direction::SOUTH, Direction::SOUTH,
            Direction::SOUTH, Direction::SOUTH,
            Direction::SOUTH, Direction::SOUTH
        },
        options);
}

VoxelModel VoxelModel::merge(std::span<VoxelModel> models) {
    ASSERT(models.size() != 0);

    std::vector<AABB> aabbs;
    std::vector<VoxelShape> shapes;

    MeshBuffer<VertexType> mesh;
    for (const auto &m : models) {
        aabbs.push_back(m.bounds);
        shapes.push_back(m.shape);
        mesh.append(m.mesh);
    }

    return VoxelModel {
        AABB::merge(std::span(std::as_const(aabbs))),
        VoxelShape::merge(std::span(shapes)),
        mesh
    };
}

// load palette from .vox file
VoxelModel::VoxelPalette VoxelModel::load_palette(const std::string &path) {
    const auto buffer = file::read_file(path).unwrap();
    const auto *scene = ogt_vox_read_scene(&buffer[0], buffer.size());

    auto data = std::vector<u32>(VoxelPalette::SIZE.x * VoxelPalette::SIZE.y);
    for (usize y = 0; y < VoxelPalette::SIZE.y; y++) {
        for (usize x = 0; x < VoxelPalette::SIZE.x; x++) {
            const auto i = (y * VoxelPalette::SIZE.x) + x;
            const auto c = scene->palette.color[i];
            data[i] =
                PixelFormat::from_rgba(
                    PixelFormat::DEFAULT,
                    (u32(c.r) << 0)
                    | (u32(c.g) << 8)
                    | (u32(c.b) << 16)
                    | (u32(0xFF) << 24));
        }
    }

    const auto name = "voxel_palette_" + path;
    const auto &entry =
        TextureAtlas::get().add(
            name,
            reinterpret_cast<u8*>(&data[0]),
            VoxelPalette::SIZE,
            VoxelPalette::SIZE);

    LOG("Created voxel palette {} at {}", name, entry.offset);
    return VoxelModel::VoxelPalette { &entry };
}

// TODO: double check this with ASAN on
VoxelModel VoxelModel::load(
    const std::string &path,
    const VoxelPalette &palette) {
    const auto buffer = file::read_file(path).unwrap();
    const auto *scene = ogt_vox_read_scene(&buffer[0], buffer.size());
    ASSERT(
        scene->num_models == 1,
        "Too many models {} in voxel model {}",
        scene->num_models, path);
   ASSERT(
        scene->num_instances == 1,
        "Too many instances {} in voxel model {}",
        scene->num_instances, path);
   ASSERT(
        scene->num_groups == 1,
        "Too many groups {} in voxel model {}",
        scene->num_groups, path);
    const auto *model = scene->models[0];

    // swap y and z, our y is up
    const auto get_model_data = [&](const auto &i) {
        const auto j = std::remove_reference_t<decltype(i)>(i.x, i.z, i.y);
        return
            model->voxel_data[
                j.x
                    + (j.y * model->size_y)
                    + (j.z * model->size_x * model->size_y)];
    };

    // compute model into shape
    // note, y and z are swapped again
    auto shape =
        VoxelShape(uvec3(model->size_x, model->size_z, model->size_y));
    shape.each(
        [&](const ivec3 &p) {
            shape[p] = get_model_data(p) != 0;
        });

    const auto palette_area = (*palette.entry)[{ 0, 0 }];
    const auto result =
        VoxelModel::make(
            shape,
            [&](Direction::Enum d, const uvec3 &p, const uvec3 &q) {
               return get_model_data(p) == get_model_data(q);
            },
            [&](Direction::Enum d,
                const uvec3 &p,
                const uvec3 &v,
                const uvec2 &s) {
                const auto value = get_model_data(p);
                const auto offset_px =
                    uvec2(
                        value % VoxelPalette::SIZE.x,
                        value / VoxelPalette::SIZE.y);
                const auto st =
                    palette_area.min + (vec2(offset_px) * palette_area.texel);
                return AABB2(st, st + (palette_area.texel / 2.0f));
            },
            [&](Direction::Enum d,
                const uvec3 &p,
                const uvec3 &v,
                const uvec2 &s) {
                // TODO: specular support in loaded voxel models
                return AABB2();
            });

    ogt_vox_destroy_scene(scene);
    return result;
}
