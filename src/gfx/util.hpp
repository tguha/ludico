#pragma once

#include "ext/stb_image.h"
#include "ext/stb_image_write.h"
#include "ext/inplace_function.hpp"
#include "util/types.hpp"
#include "util/util.hpp"
#include "util/file.hpp"
#include "util/assert.hpp"
#include "util/resource.hpp"

#include "gfx/bgfx.hpp"
#include "gfx/pixel_format.hpp"

// TODO: cleanup

constexpr inline auto
    BGFX_FLAGS_DEFAULT =
        BGFX_STATE_WRITE_MASK
            | BGFX_STATE_DEPTH_TEST_LESS
            | BGFX_STATE_CULL_CCW;

struct RenderState {
    bgfx::ViewId view = 0;
    u64 flags = 0, bgfx = 0;

    RenderState() = default;
    explicit RenderState(bgfx::ViewId view, u64 flags = 0, u64 bgfx = 0)
        : view(view), flags(flags), bgfx(bgfx) {}

    inline RenderState or_defaults() const {
        RenderState r = *this;
        if (!r.bgfx) {
            r.bgfx = BGFX_FLAGS_DEFAULT;
        }
        return r;
    }
};

struct Program;

using RenderFn = std::function<void(RenderState)>;
using InplaceRenderFn =
    InplaceFunction<void(RenderState), 32>;
constexpr inline auto RENDER_FN_EMPTY =
    [](RenderState _){};

using PrepareFn =
    std::function<void(const Program&, mat4&, RenderState&)>;
using InplacePrepareFn =
    InplaceFunction<void(const Program&, mat4&, RenderState&), 32>;
constexpr inline auto PREPARE_FN_EMPTY =
    [](const Program &_, mat4 &__, RenderState &___){};

namespace gfx {
inline std::string get_platform_shader(
    const std::string &path) {
    std::string platform;

    switch (bgfx::getRendererType()) {
        case bgfx::RendererType::Noop:
        case bgfx::RendererType::Direct3D9:
            platform = "dx9";
            break;
        case bgfx::RendererType::Direct3D11:
        case bgfx::RendererType::Direct3D12:
            platform = "dx11";
            break;
        case bgfx::RendererType::Agc:
        case bgfx::RendererType::Gnm:
            platform = "pssl";
            break;
        case bgfx::RendererType::Metal:
            platform = "metal";
            break;
        case bgfx::RendererType::Nvn:
            platform = "nvn";
            break;
        case bgfx::RendererType::OpenGL:
            platform = "glsl";
            break;
        case bgfx::RendererType::OpenGLES:
            platform = "essl";
            break;
        case bgfx::RendererType::Vulkan:
            platform = "spirv";
            break;
        case bgfx::RendererType::WebGPU:
            platform = "spirv";
            break;
        default:
            ASSERT(false, "Renderer type out of range");
            break;
}

    const auto[base, filename, ext] = file::split_path(path);
    return base + "/" + filename + "." + platform + ".bin";
}

inline result::Result<bgfx::ShaderHandle, std::string> load_shader(
    const std::string &path) {
    auto read = file::read_string(path);

    if (read.isErr()) {
        return result::Err(read.unwrapErr());
    }

    auto text = read.unwrap();
    auto res =
        bgfx::createShader(
            bgfx::copy(text.c_str(), text.length()));
    bgfx::setName(res, path.c_str());
    return result::Ok(res);
}

// write texture data to disk
// NOTE: expects RGBA data with no row padding
inline result::Result<void, std::string>
    write_texture_data(
        const std::string &path,
        const uvec2 &size,
        const u8 *data) {
        stbi_flip_vertically_on_write(true);
    int res =
        stbi_write_png(
            path.c_str(),
            size.x,
            size.y,
            4, // RGBA
            reinterpret_cast<const void*>(data),
            size.x * 4);

    if (!res) {
        return result::Err(fmt::format("Error writing texture to {}", path));
    }

    return result::Ok();
}

inline result::Result<std::tuple<uvec2, u8*>, std::string> load_texture_data(
    const std::string &path,
    PixelFormat::Enum pixel_format = PixelFormat::DEFAULT) {
    uvec2 size;
    int channels;
    stbi_set_flip_vertically_on_load(true);

    ivec2 s;
    u8 *data = stbi_load(path.c_str(), &s.x, &s.y, &channels, 0);
    size = uvec2(s);

    if (!data) {
        return result::Err("Error loading texture " + path);
    }

    ASSERT(channels == 3 || channels == 4);

    // pad with A = 0xFF
    if (channels == 3) {
        LOG("Image at {} is RGB, padding to RGBA", path);

        u8 *new_data =
            reinterpret_cast<u8*>(std::malloc(size.x * size.y * 4));

        for (usize y = 0; y < size.y; y++) {
            for (usize x = 0; x < size.x; x++) {
                for (usize i = 0; i < 3; i++) {
                    new_data[((y * size.x) + x) * 4 + i] =
                        data[((y * size.x) + x) * 3 + i];
                }

                new_data[((y * size.x) + x) * 4 + 3] = 0xFF;
            }
        }

        std::free(data);
        data = new_data;
    }

    // convert data from RGBA to target pixel format
    for (isize i = 0; i < s.x * s.y * 4; i += 4) {
        *reinterpret_cast<u32*>(&data[i])
            = PixelFormat::from_rgba(
                pixel_format,
                *reinterpret_cast<u32*>(&data[i]));
    }

    return result::Ok(std::make_tuple(size, data));
}

inline auto load_texture(
    const std::string &path,
    u8 **data_out = nullptr,
    PixelFormat::Enum format = PixelFormat::DEFAULT)
        -> result::Result<std::tuple<uvec2, bgfx::TextureHandle>, std::string> {

    const auto data_res = load_texture_data(path, format);

    if (data_res.isErr()) {
        return result::Err(data_res.unwrapErr());
    }

    auto [size, data] = data_res.unwrap();
    const auto res =
        bgfx::createTexture2D(
            size.x, size.y,
            false, 1,
            PixelFormat::to_texture_format(format),
            BGFX_SAMPLER_U_CLAMP
                | BGFX_SAMPLER_V_CLAMP
                | BGFX_SAMPLER_MIN_POINT
                | BGFX_SAMPLER_MAG_POINT,
            bgfx::copy(data, size.x * size.y * 4));

    if (!bgfx::isValid(res)) {
        return result::Err("Error loading texture " + path);
    }

    if (data_out) {
        *data_out = data;
    } else {
        std::free(data);
    }

    return result::Ok(std::tuple(size, res));
}

template <typename T>
inline RDResource<T> as_bgfx_resource(T &&t) {
    return RDResource<T>(
        std::move(t),
        [](T &handle) { bgfx::destroy(handle); });
}

template <typename V, size_t N, size_t M>
inline std::tuple<
    RDResource<bgfx::VertexBufferHandle>,
    RDResource<bgfx::IndexBufferHandle>> create_buffers(
        std::span<const V, N> vertices, std::span<const u16, M> indices,
        u64 vertex_flags = 0, u64 index_flags = 0) {
    return std::make_tuple(
        as_bgfx_resource(
            bgfx::createVertexBuffer(
                bgfx::copy(&vertices[0], vertices.size() * sizeof(V)),
                V::layout, vertex_flags)),
        as_bgfx_resource(
            bgfx::createIndexBuffer(
                bgfx::copy(&indices[0], indices.size() * sizeof(u16)),
                index_flags)));
}
}
