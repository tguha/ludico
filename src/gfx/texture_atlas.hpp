#pragma once

#include "util/types.hpp"
#include "util/util.hpp"
#include "util/assert.hpp"
#include "util/math.hpp"
#include "util/aabb.hpp"

#include "gfx/texture.hpp"
#include "gfx/util.hpp"
#include "gfx/pixel_format.hpp"
#include "gfx/pixels.hpp"

// forward declaration
struct MultiTexture;

// TODO: split out into another header?
struct TextureArea {
    const MultiTexture *parent;
    vec2 min, max, sprite_unit, texel;

    TextureArea() = default;
    TextureArea(
        const MultiTexture *parent,
        const vec2 &min,
        const vec2 &max,
        const vec2 &sprite_unit,
        const vec2 &texel)
        : parent(parent),
          min(min),
          max(max),
          sprite_unit(sprite_unit),
          texel(texel) {}

    inline TextureArea merge(const TextureArea &other) const {
        ASSERT(this->parent == other.parent);
        const auto boxes =
            types::make_array(
                static_cast<AABB2>(*this),
                static_cast<AABB2>(other));
        const auto box = AABB2::merge(std::span(boxes));
        return TextureArea(
            this->parent,
            box.min,
            box.max,
            this->sprite_unit,
            this->texel);
    }

    inline operator AABB2() const {
        return AABB2(this->min, this->max);
    }

    // convert min/max to pixel bounds
    inline auto pixels() const {
        return AABB2u(
            uvec2(this->min / this->texel),
            uvec2(this->max / this->texel) - uvec2(1));
    }

    inline auto size() const {
        return this->pixels().size();
    }

    // take a pixel-subsection of these coords
    auto subpixels(const AABB2u &bounds) const {
        TextureArea res = *this;
        res.min = this->min + (vec2(bounds.min) * this->texel);
        res.max = this->min + (vec2(bounds.max + 1u) * this->texel);
        return res;
    }

    // take a pixel-subsection of these atlas coords, of the specified size
    // centered in the coords of this tile
    auto centered_subpixels(const uvec2 &size) const {
        const AABB2u area = this->pixels();
        const uvec2 os = uvec2(area.size() - size) / uvec2(2);
        return this->subpixels(AABB2u(os, os + size - 1u));
    }

    // take a pixel-subsection of these atlas coords, using
    // (size[axis_0], size[axis_1]) as the bounds centered inside the coords
    // selected by this tile
    auto centered_subpixels(
        const uvec3 &size, int axis_0, int axis_1) const {
        return this->centered_subpixels(uvec2(size[axis_0], size[axis_1]));
    }
};

// TODO: split out into another header?
struct MultiTexture {
public:
    // set in subclass
    u8 *data;
    uvec2 unit_size, size, data_size, offset, size_units;
    vec2 texel, unit_texel;

    inline MultiTexture with_unit_size(const uvec2 &new_unit_size) const {
        MultiTexture m = *this;
        m.unit_size = new_unit_size;
        m.size_units = m.size / m.unit_size;
        m.unit_texel = vec2(m.unit_size) * m.texel;
        return m;
    }

    // retrieve pixels from the specified coordinates
    inline void pixels(
        std::span<u8> dest, const TextureArea &area) const {
        const auto src = this->data;
        const auto src_size = this->data_size;

        const auto bounds = area.pixels();
        const auto size = bounds.size();

        ASSERT(dest.size() >= static_cast<usize>(size.y * size.x * 4));

        for (usize y = 0; y < size.y; y++) {
            for (usize x = 0; x < size.x; x++) {
                const auto
                    i_src =
                        ((bounds.min.y + y) * src_size.x) + (bounds.min.x + x),
                    i_dst = (y * size.x) + x;

                for (usize i = 0; i < 4; i++) {
                    dest[(i_dst * 4) + i] = src[(i_src * 4) + i];
                }
            }
        }
    }

    inline TextureArea to_area() const {
        return (*this)[AABB2u(uvec2(0), this->size_units - 1u)];
    }

    inline AABB2u as_aabb() const {
        return AABB2u(this->size - 1u).translate(this->offset);
    }

    inline TextureArea operator[](const uvec2 &i) const {
        const auto min =
            vec2((i * this->unit_size) + this->offset) * this->texel;
        return TextureArea(
            this,
            min,
            min + this->unit_texel,
            this->unit_texel, this->texel);
    }

    template <typename T, std::size_t N>
        requires (std::is_integral<T>::value)
    inline TextureArea operator[](const T(&list)[N]) const {
        static_assert(N == 2, "must use exactly 2 indices");
        return (*this)[uvec2(list[0], list[1])];
    }

    // inclusive bounds
    inline TextureArea operator[](const AABB2u &box) const {
        return ((*this)[box.min]).merge((*this)[box.max]);
    }
};

struct TextureAtlas;

// entry (labeled area of pixels) in texture atlas
struct TextureAtlasEntry : public MultiTexture {
    TextureAtlas *atlas;
    std::string label;
    AABB2u bounds;

    // optional: info from sprite loader
    std::optional<std::string> path;

    TextureAtlasEntry() = default;
    TextureAtlasEntry(const TextureAtlasEntry&) = delete;
    TextureAtlasEntry(TextureAtlasEntry&&) = default;
    TextureAtlasEntry &operator=(const TextureAtlasEntry&) = delete;
    TextureAtlasEntry &operator=(TextureAtlasEntry&&) = default;

    TextureAtlasEntry(
        TextureAtlas &atlas,
        const std::string &label,
        const AABB2u &bounds,
        const uvec2 &offset,
        const uvec2 &size,
        const uvec2 &unit_size);

    inline Pixels as_pixels() const;

    TextureArea as_area() const;

    // re-upload this area to the GPU
    void upload() const;
};

// TODO: allocates immediately 256 MiB on startup since we persist the entire
// atlas in memory. maybe there's a better way to do this?
struct TextureAtlas {
    struct Animation {
        uvec2 origin;
        uvec2 size;
        usize length;
        usize frame_ticks;
        bool vertical;
    };

    static constexpr auto PIXEL_FORMAT = PixelFormat::DEFAULT;

    // get TextureAtlas::get()
    static TextureAtlas &get();

    Texture texture;

    // TODO: store contiguously in memory
    std::unordered_map<std::string, std::unique_ptr<TextureAtlasEntry>> entries;

    std::vector<Animation> animations;

    // mirrors atlas data on GPU (must be saved for voxel models, etc.)
    // NOTE: DOES NOT VARY with animations! only the GPU texture will do that
    std::unique_ptr<u8[]> data;

    // size of whole atlas
    uvec2 size;

    TextureAtlas() = default;
    explicit TextureAtlas(const uvec2 &size);

    inline operator const Texture&() const {
        return this->texture;
    }

    operator bgfx::TextureHandle() const;

    inline bool contains(const std::string &label) const {
        return this->entries.contains(label);
    }

    inline const TextureAtlasEntry &operator[](
        const std::string &label) const {
        ASSERT(
            this->entries.contains(label),
            "Atlas does not contain " + label);
        return *(this->entries.at(label));
    }

    inline vec2 texel() const {
        return 1.0f / vec2(this->size);
    }

    // updates animated sections
    void tick();


    // allocates space for an entry with the specified size in the atlas
    TextureAtlasEntry &alloc(
        const std::string &label,
        const uvec2 &size,
        const uvec2 &unit_size);

    // adds an entry with the specified data into the atlas
    // NOTE: data_i is copied
    TextureAtlasEntry &add(
        const std::string &label,
        const u8 *data_i,
        const uvec2 &size_i,
        const uvec2 &unit_size);

    // animate an entry, starting at the specified offset into the entry
    // length is animation length
    // frame_ticks are ticks per each frame of animation
    // if vertical, animation travels vertically, otherwise frames are seeked
    // horizontally
    void animate(
        const TextureAtlasEntry &entry,
        const uvec2 &offset,
        usize length,
        usize frame_ticks,
        bool vertical);

private:
    friend struct TextureAtlasEntry;

    // upload a subsection of the atlas to the GPU
    void upload(const AABB2u &box) const;
};
