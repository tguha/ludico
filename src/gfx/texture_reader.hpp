#pragma once

#include "gfx/pixels.hpp"
#include "util/types.hpp"
#include "util/math.hpp"
#include "util/util.hpp"
#include "gfx/bgfx.hpp"
#include "gfx/texture.hpp"
#include "ext/inplace_function.hpp"

// automated texture reader with automatically blitting
// reads data back from GPU rendered textures, maybe with a slight delay :)
// NOTE: (0, 0) is bottom LEFT for uvec2 indexing!
// TODO: convert to fully to PixelFormat
struct TextureReader {
    using GetFrameFn = InplaceFunction<usize(void), 1>;

    const Texture *texture;
    Texture blit;

    bgfx::TextureFormat::Enum format;
    bgfx::ViewId view;

    GetFrameFn get_frame_fn;
    usize request_frame = 0, ready_frame = 0;
    std::unique_ptr<u8[]> front, back;
    bool requested = false;

    // size of data unit according to texture format
    usize data_size;

    TextureReader() = default;
    TextureReader(const TextureReader&) = delete;
    TextureReader(TextureReader&&) = default;
    TextureReader &operator=(const TextureReader&) = delete;
    TextureReader &operator=(TextureReader&&) = default;

    TextureReader(
        const Texture &texture,
        bgfx::ViewId view,
        bgfx::TextureFormat::Enum format,
        GetFrameFn &&get_frame_fn)
        : texture(&texture),
          format(format),
          view(view),
          get_frame_fn(std::move(get_frame_fn)) {
        // TODO: support more data sizes
        switch (format) {
            case bgfx::TextureFormat::RGBA32F:
                this->data_size = 16;
                break;
            case bgfx::TextureFormat::BGRA8:
            case bgfx::TextureFormat::RGBA8:
            case bgfx::TextureFormat::R32F:
                this->data_size = 4;
                break;
            default:
                ASSERT(false, "unsupported format");
                break;
        }
        const auto size = texture.size.x * texture.size.y * this->data_size;
        this->front = std::unique_ptr<u8[]>(new u8[size]);
        this->back = std::unique_ptr<u8[]>(new u8[size]);
        this->blit = Texture(
            bgfx::createTexture2D(
                texture.size.x, texture.size.y,
                false, 1, format,
                 BGFX_SAMPLER_MIN_POINT
                    | BGFX_SAMPLER_MAG_POINT
                    | BGFX_SAMPLER_MIP_POINT
                    | BGFX_SAMPLER_U_CLAMP
                    | BGFX_SAMPLER_V_CLAMP
                    | BGFX_TEXTURE_READ_BACK
                    | BGFX_TEXTURE_BLIT_DST),
            texture.size);

        // configure blit view
        bgfx::setViewClear(
            this->view, BGFX_CLEAR_COLOR, 0xFF00FFF);
        bgfx::setViewRect(
            this->view, 0, 0, texture.size.x, texture.size.y);
    }

    void update() {
        const auto frame = this->get_frame_fn();

        // intentionally don't request for single frames in case this is a
        // render target (one frame where requested = false)
        if (frame >= this->ready_frame && this->requested) {
            std::swap(this->front, this->back);
            this->requested = false;
        } else if (!this->requested) {
            bgfx::blit(
                this->view,
                this->blit, 0, 0,
                *this->texture, 0, 0,
                this->texture->size.x, this->texture->size.y);
            this->ready_frame =
                bgfx::readTexture(this->blit, this->back.get());
            this->request_frame = frame;
            this->requested = true;
        }
    }

    template <typename D>
    inline D get(const uvec2 &i) const {
        return this->get<D>(
            (texture->size.y - i.y - 1) * texture->size.x + i.x);
    }

    template <typename D>
    inline D get(usize index) const {
        const auto size = this->texture->size.x * this->texture->size.y;
        ASSERT(index < size);
        u8 *data = &this->front[index * this->data_size];

        if constexpr (std::is_same_v<D, vec4>) {
            ASSERT(this->format == bgfx::TextureFormat::RGBA32F);
            return *reinterpret_cast<vec4*>(data);
        } else if constexpr (std::is_same_v<D, u32>) {
            return PixelFormat::to_rgba(
                this->format == bgfx::TextureFormat::BGRA8 ?
                    PixelFormat::BGRA : PixelFormat::RGBA,
                    *reinterpret_cast<u32*>(data));
        } else if constexpr (std::is_same_v<D, f32>) {
            ASSERT(this->format == bgfx::TextureFormat::R32F);
            return *reinterpret_cast<f32*>(data);
        }
    }
};
