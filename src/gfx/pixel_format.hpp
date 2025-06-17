#pragma once

#include "gfx/bgfx.hpp"
#include "util/types.hpp"
#include "util/math.hpp"

// "RGBA" is assumed to mean R as lowest order byte and A as highest order byte.
// same goes for BGRA.
// rgba: 0xaabbggrr
// bgra: 0xaarrggbb
struct PixelFormat {
    enum Enum {
        BGRA = 0,
        RGBA = 1
    };

    static constexpr auto DEFAULT = PixelFormat::BGRA;

    static inline bgfx::TextureFormat::Enum to_texture_format(Enum e) {
        switch (e) {
            case BGRA: return bgfx::TextureFormat::BGRA8;
            case RGBA: return bgfx::TextureFormat::RGBA8;
        }
    }

    template <math::length_t L, class T, math::qualifier Q>
    static inline u32 from_vec(Enum e, const math::vec<L, T, Q> &v) {
        u32 r = 0;
        r |= (static_cast<u8>(v.r * 255.0f) <<  0);
        r |= (static_cast<u8>(v.g * 255.0f) <<  8);
        r |= (static_cast<u8>(v.b * 255.0f) << 16);

        if constexpr (L == 4) {
            r |= (static_cast<u8>(v.a * 255.0f) << 24);
        } else {
            r |= (0xFF << 24);
        }
        return from_rgba(e, r);
    }

    template <typename V, typename Traits = math::vec_traits<V>>
        requires (Traits::L == 3 || Traits::L == 4)
    static inline V to_vec(Enum e, u32 p) {
        const auto rgba = to_rgba(e, p);

        V v;

        const auto assign =
            [&](usize index_rgba, usize index_vec) {
                const auto x = (rgba >> (8 * index_rgba)) & 0xFF;
                if constexpr (std::is_integral_v<typename Traits::T>) {
                    v[index_vec] = x;
                } else {
                    v[index_vec] = x / 255.0f;
                }
            };

        assign(0, 0);
        assign(1, 1);
        assign(2, 2);

        if constexpr (Traits::L == 4) {
            assign(3, 3);
        }

        return v;
    }

    static inline u32 from_rgba(Enum e, u32 rgba) {
        switch (e) {
            case BGRA:
                return (rgba & 0xFF00FF00)
                    | ((rgba >> 16) & 0xFF)
                    | ((rgba & 0xFF) << 16);
            case RGBA:
                return rgba;
        }
    }

    static inline u32 to_rgba(Enum e, u32 px) {
        switch (e) {
            case BGRA:
                return (px & 0xFF00FF00)
                    | ((px >> 16) & 0xFF)
                    | ((px & 0xFF) << 16);
            case RGBA:
                return px;
        }
    }
};
