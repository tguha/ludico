#pragma once

#include "util/util.hpp"
#include "util/types.hpp"
#include "util/math.hpp"
#include "gfx/util.hpp"
#include "gfx/pixel_format.hpp"

struct Image {
    enum ScalingType {
        SCALE_NEAREST,
        SCALE_BILINEAR
    };

    uvec2 size;

    // image data buffer
    std::unique_ptr<u8[]> data;

    // image format
    PixelFormat::Enum format;

    Image() = default;

    explicit Image(
        const std::string &path,
        PixelFormat::Enum format = PixelFormat::DEFAULT)
        : format(format) {
        const auto [size, data] = gfx::load_texture_data(path, format).unwrap();
        this->size = size;
        this->data = std::unique_ptr<u8[]>(data);
    }

    explicit Image(
        const uvec2 &size,
        const u8 *data = nullptr,
        PixelFormat::Enum format = PixelFormat::DEFAULT)
        : size(size),
          format(format) {
        this->data = std::unique_ptr<u8[]>(new u8[math::prod(size) * 4]);

        if (data) {
            std::memcpy(&this->data[0], &data[0], math::prod(size) * 4);
        }
    }

    explicit Image(const std::tuple<uvec2, u8*> &t)
        : Image(std::get<0>(t), std::get<1>(t)) {}

    Image(
        const uvec2 &size,
        std::span<const u8> span,
        PixelFormat::Enum format = PixelFormat::DEFAULT)
        : Image(size, &span[0], format) {}

    template <typename T, std::size_t N>
        requires (std::is_integral<T>::value)
    inline auto &operator[](const T(&list)[N]) const {
        static_assert(N == 2, "must use exactly 2 indices");
        return (*this)[uvec2(list[0], list[1])];
    }

    template <typename T, typename Traits = math::vec_traits<T>>
        requires (std::is_integral_v<typename Traits::T> && Traits::L == 2)
    inline u32 &operator[](const T &i) const {
        return (*this)[i.y * this->size.x + i.x];
    }

    inline u32 &operator[](usize i) const {
        return reinterpret_cast<u32*>(&this->data[0])[i];
    }

    template <typename T, std::size_t N>
        requires (std::is_integral<T>::value)
    inline auto &byte(const T(&list)[N]) const {
        static_assert(N == 2, "must use exactly 2 indices");
        return this->byte(uvec2(list[0], list[1]));
    }

    template <typename T, typename Traits = math::vec_traits<T>>
        requires (std::is_integral_v<typename Traits::T> && Traits::L == 2)
    inline u8 &byte(const T &i) const {
        return this->byte(i.y * this->size.x + i.x);
    }

    inline u8 &byte(usize i) const {
        return this->data[i];
    }

    template <typename T, typename Traits = math::vec_traits<T>>
        requires (std::is_integral_v<typename Traits::T> && Traits::L == 2)
    inline vec4 rgba(const T &i) const {
        return PixelFormat::to_vec<vec4>(
            this->format,
            (*this)[i.y * this->size.x + i.x]);
    }

    inline vec4 rgba(usize i) const {
        return PixelFormat::to_vec<vec4>(
            this->format,
            (*this)[i]);
    }

    template <typename t, std::size_t n>
        requires (std::is_integral<t>::value)
    inline auto rgba(const t(&list)[n]) const {
        static_assert(n == 2, "must use exactly 2 indices");
        return this->rgba(uvec2(list[0], list[1]));
    }

    template <typename T, typename Traits = math::vec_traits<T>>
        requires (std::is_integral_v<typename Traits::T> && Traits::L == 2)
    inline vec3 rgb(const T &i) const {
        return this->rgba(i).rgb();
    }

    inline vec3 rgb(usize i) const {
        return this->rgba(i).rgb();
    }

    template <typename t, std::size_t n>
        requires (std::is_integral<t>::value)
    inline auto rgb(const t(&list)[n]) const {
        static_assert(n == 2, "must use exactly 2 indices");
        return this->rgb(uvec2(list[0], list[1]));
    }

    Image clone() const {
        Image result(this->size);
        std::memcpy(
            &result.data[0],
            &this->data[0],
            math::prod(this->size) * 4);
        return result;
    }

    // convert image to differnt format
    Image formatted(PixelFormat::Enum format) const {
        if (format == this->format) {
            return this->clone();
        }

        Image result = this->clone();
        for (usize i = 0; i < math::prod(this->size); i++) {
            result[i] =
                PixelFormat::from_rgba(
                    format,
                    PixelFormat::to_rgba(
                        this->format,
                        (*this)[i]));
        }
        return result;
    }

    // scales image to specific size
    Image scaled(const uvec2 &size, ScalingType type = SCALE_NEAREST) const {
        return this->scaled(vec2(size) / vec2(this->size), type);
    }

    // scales image using nearest neighbor filtering
    // TODO: this is broken!
    Image scaled(const vec2 &scale, ScalingType type = SCALE_NEAREST) const {
        Image result(uvec2(vec2(this->size) * scale));

        int samples = 1;
        switch (type) {
            case SCALE_NEAREST: break;
            case SCALE_BILINEAR: samples = 2;
        }

        // samples this image at a normalized coordinate
        const auto sample =
            [&](const vec2 &p) {
                const auto q = math::clamp(p, vec2(0.0f), vec2(1.0f));
                return this->rgba(
                    uvec2(math::round(q * vec2(this->size - 1u))));
            };

        const auto inv_scale = vec2(1.0f) / scale;
        const auto unit_old = (1.0f / vec2(this->size));
        for (usize y = 0; y < result.size.y; y++) {
            for (usize x = 0; x < result.size.x; x++) {
                const auto p = uvec2(x, y);

                if (type == SCALE_NEAREST) {
                    result[{ x, y }] =
                        (*this)[
                            math::min(
                                uvec2(math::round(vec2(p) * inv_scale)),
                                this->size - 1u)];
                } else {
                    // convet p -> normalized coordinates
                    const auto n = (1.0f / vec2(result.size)) * vec2(p);

                    vec4 value = vec4(0.0);

                    // sample around p to get final value
                    const auto base =
                        n - (vec2(math::floor(samples / 2.0f)) * unit_old);
                    for (int t = 0; t < samples; t++) {
                        for (int s = 0; s < samples; s++) {
                            value += sample(base + (unit_old * vec2(s, t)));
                        }
                    }

                    result[{ x, y }] =
                        PixelFormat::from_vec(
                            this->format,
                            value / vec4(math::pow(samples, 2.0f)));
                }
            }
        }

        return result;
    }

    // write image to path
    void write(const std::string &path, bool overwrite = false) {
        ASSERT(!file::exists(path) || overwrite, "cannot overwrite {}", path);
        Image rgba = this->formatted(PixelFormat::RGBA);
        gfx::write_texture_data(path, this->size, &rgba.byte(0));
    }
};
