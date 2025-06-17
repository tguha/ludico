#pragma once

#include "util/util.hpp"
#include "util/types.hpp"
#include "util/aabb.hpp"
#include "util/assert.hpp"
#include "gfx/pixel_format.hpp"

struct TextureArea;

// rgba pixel array wrapper
struct Pixels {
    struct RGBA {
        Pixels *p;
        u8 *index;

        inline operator vec4() const {
            u32 d = static_cast<u32>(*this);
            return vec4(
                ((d >>  0) & 0xFF) / 255.0f,
                ((d >>  8) & 0xFF) / 255.0f,
                ((d >> 16) & 0xFF) / 255.0f,
                ((d >> 24) & 0xFF) / 255.0f);
        }

        inline operator u32() const {
            return PixelFormat::to_rgba(
                PixelFormat::DEFAULT,
                *reinterpret_cast<const u32*>(this->index));
        }

        inline RGBA &operator=(const vec4 &rhs) {
            u32 rgba = 0;
            for (usize i = 0; i < 4; i++) {
                rgba |= (u8(rhs[i] * 255.0f) << (i * 8));
            }
            *reinterpret_cast<u32*>(this->index) =
                PixelFormat::from_rgba(
                    PixelFormat::DEFAULT, rgba);
            return *this;
        }


        inline RGBA &operator=(const u32 &rhs) {
            *reinterpret_cast<u32*>(this->index) =
                PixelFormat::from_rgba(
                    PixelFormat::DEFAULT, rhs);
            return *this;
        }
    };

    u8 *data;
    uvec2 size;
    uvec2 offset;
    uvec2 data_size;

    Pixels() = default;
    Pixels(
        u8 *data,
        const uvec2 &size,
        const uvec2 &offset,
        const uvec2 data_size)
        : data(data), size(size), offset(offset), data_size(data_size) {}
    Pixels(
        u8 *data,
        const uvec2 &size)
        : Pixels(data, size, uvec2(0), size) {}

    Pixels(const TextureArea &area);

    inline auto *addr(usize i) const {
        const usize x = i % size.x, y = i / size.x;
        return &data[((y + offset.y) * data_size.x + (x + offset.x)) * 4];
    }

    inline auto *addr(const uvec2 &i) const {
        return this->addr(i.y * size.x + i.x);
    }

public:

    inline Pixels subpixels(
        uvec2 offset, uvec2 size) {
        ASSERT(size.x + offset.x <= this->size.x);
        ASSERT(size.y + offset.y <= this->size.y);
        return Pixels(
            this->data, size, this->offset + offset, this->data_size);
    }

    inline auto &width() const { return size.x; }
    inline auto &height() const { return size.y; }

    inline auto operator[](usize i) {
        return PixelFormat::to_rgba(
            PixelFormat::DEFAULT,
            *reinterpret_cast<u32*>(this->addr(i)));
    }

    inline auto operator[](usize i) const {
        return (*const_cast<Pixels*>(this))[i];
    }

    inline auto operator[](const uvec2 &i) {
        return (*this)[i.y * size.x + i.x];
    }

    inline auto operator[](const uvec2 &i) const {
        return (*this)[i.y * size.x + i.x];
    }

    inline RGBA rgba(const uvec2 &i) {
        return RGBA { this, this->addr(i) };
    }

    inline RGBA rgba(const uvec2 &i) const {
        auto *_this = const_cast<Pixels*>(this);
        return RGBA { _this, _this->addr(i) };
    }

    inline AABB2u as_aabb() const {
        return AABB2u(this->size - 1u).translate(this->offset);
    }
};

// pixel manipulation/reading utilities
namespace pixels {
inline vec4 rgba_to_vec4(u32 rgba) {
    vec4 res;
    UNROLL(4)
    for (usize i = 0; i < 4; i++) {
        res[i] = u8(rgba >> (i * 8)) / 255.0f;
    }
    return res;
}

inline u32 vec4_to_rgba(const vec4 &v) {
    u32 res = 0;
    UNROLL(4)
    for (usize i = 0; i < 4; i++) {
        res |= u32(v[i] * 255.0f) << (i * 8);
    }
    return res;
}

// get all colors present in a sprite
inline std::unordered_set<vec4> colors(Pixels pixels) {
    std::unordered_set<u32> colors;

    for (usize y = 0; y < pixels.height(); y++) {
        for (usize x = 0; x < pixels.width(); x++) {
            const auto rgba = pixels.rgba({ x, y });

            // TODO: maybe include non-solid colors?
            if ((u32(rgba) >> 24) != 0xFF) {
                continue;
            }

            auto it = colors.find(rgba);
            if (it == colors.end()) {
                colors.insert(it, rgba);
            }
        }
    }

    std::unordered_set<vec4> res;
    std::transform(
        colors.begin(), colors.end(), std::inserter(res, std::end(res)),
        [](u32 c) { return rgba_to_vec4(c); });
    return res;
}

inline void copy(const Pixels &dst, const Pixels &src) {
    ASSERT(dst.size.x <= src.size.x);
    ASSERT(dst.size.y <= src.size.y);

    for (usize y = 0; y < dst.size.y; y++) {
        std::memcpy(
            &dst.data[
                (((dst.offset.y + y) * dst.data_size.x) + dst.offset.x) * 4],
            &src.data[
                (((src.offset.y + y) * src.data_size.x) + src.offset.x) * 4],
            dst.size.x * 4);
    }
}

template <typename F>
    requires (requires (F f) { { f(std::declval<u32>()) } -> std::same_as<bool>; })
inline void copy_if(const Pixels &dst, const Pixels &src, F &&filter) {
    ASSERT(dst.size.x <= src.size.x);
    ASSERT(dst.size.y <= src.size.y);

    for (usize y = 0; y < dst.size.y; y++) {
        for (usize x = 0; x < dst.size.x; x++) {
            const auto i_src =
                (((src.offset.y + y) * src.data_size.x)
                    + (src.offset.x + x)) * 4;

            if (!filter(*reinterpret_cast<u32*>(&src.data[i_src]))) {
                continue;
            }

            const auto i_dst =
                (((dst.offset.y + y) * dst.data_size.x)
                    + (dst.offset.x + x)) * 4;

            *reinterpret_cast<u32*>(&dst.data[i_dst]) =
                *reinterpret_cast<u32*>(&src.data[i_src]);
        }
    }
}

inline void rotate(const Pixels &dst, const Pixels &src, bool ccw = true) {
    ASSERT(dst.size == src.size, "can only rotate to dst of equal size");
    ASSERT(src.size.x == src.size.y, "can only rotate square regions");

    for (usize i = 0; i < src.size.x; i++) {
        for (usize j = 0; j < src.size.y; j++) {
            *dst.addr({ i, j }) = *src.addr({ src.size.x - j - 1, i });
        }
    }
}
}
