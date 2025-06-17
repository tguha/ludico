#include "gfx/renderer.hpp"
#include "gfx/bgfx.hpp"
#include "gfx/cube.hpp"
#include "gfx/game_camera.hpp"
#include "gfx/icon_renderer.hpp"
#include "gfx/texture_reader.hpp"
#include "gfx/palette.hpp"
#include "gfx/sprite_renderer.hpp"
#include "gfx/particle_renderer.hpp"
#include "gfx/font.hpp"
#include "gfx/vertex.hpp"
#include "gfx/framebuffer.hpp"
#include "gfx/program.hpp"
#include "gfx/renderer.hpp"
#include "gfx/sun.hpp"
#include "gfx/texture.hpp"
#include "gfx/pixel_format.hpp"
#include "gfx/util.hpp"
#include "gfx/entity_highlighter.hpp"
#include "gfx/debug_renderer.hpp"
#include "gfx/multi_mesh_instancer.hpp"
#include "util/rand.hpp"
#include "util/types.hpp"
#include "util/util.hpp"
#include "util/toml.hpp"
#include "util/managed_resource.hpp"
#include "util/stack_allocator.hpp"
#include "platform/platform.hpp"
#include "platform/window.hpp"
#include "occlusion_map.hpp"
#include "constants.hpp"
#include "global.hpp"

#include <backward.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "ext/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "ext/stb_image_write.h"

Renderer &Renderer::get() {
    return *global.renderer;
}

static u64 get_reset_flags(Renderer &renderer) {
    return
        (*global.settings)["gfx"]["vsync"].value_or(true) ?
            BGFX_RESET_VSYNC : BGFX_RESET_NONE;
}

static uvec2 get_render_size(Renderer &renderer, uvec2 window_size) {
    return uvec2(320, 180); //window_size / uvec2(renderer.scale_factor);
}

static constexpr u64 BUFFER_FLAGS =
    BGFX_SAMPLER_MIN_POINT
        | BGFX_SAMPLER_MAG_POINT
        | BGFX_SAMPLER_MIP_POINT
        | BGFX_SAMPLER_U_CLAMP
        | BGFX_SAMPLER_V_CLAMP;

static auto make_buffer(
    uvec2 size,
    bgfx::TextureFormat::Enum fmt,
    usize flags = BGFX_TEXTURE_RT | BUFFER_FLAGS) {
    return Texture(
        bgfx::createTexture2D(size.x, size.y, false, 1, fmt, flags),
        size);
}

static Framebuffer make_framebuffer(
    bgfx::ViewId view,
    const std::vector<bgfx::Attachment> &attachments) {
    auto framebuffer = Framebuffer(
        bgfx::createFrameBuffer(
            attachments.size(),
            &attachments[0]));
    bgfx::setViewFrameBuffer(view, framebuffer.handle);
    return framebuffer;
};

static auto make_view(
    bgfx::ViewId view,
    uvec2 size,
    u64 clear = BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
    u64 clear_color = 0) {
    bgfx::setViewClear(view, clear, clear_color, 1.0f, 0);
    bgfx::setViewRect(view, 0, 0, size.x, size.y);

    // TODO: maybe don't force all views to render sequentially, only
    // 2D/UI views (which require it for alpha blending orders)
    bgfx::setViewMode(view, bgfx::ViewMode::Sequential);
}

struct Renderer::Callbacks final : public bgfx::CallbackI {
    void fatal(
        const char *_filePath, uint16_t _line,
        bgfx::Fatal::Enum _code, const char *_str) override {
        backward::StackTrace st;
        st.load_here();
        backward::Printer p;
        p.print(st);
        ERROR(
            "[BGFX] Fatal error: 0x{:X}: {}",
            static_cast<usize>(_code),
            std::string_view(_str));
        std::exit(1);
    }

    void traceVargs(
        const char *_filePath, uint16_t _line,
        const char *_format, va_list _argList) override {
        if (!(*global.settings)["log_bgfx"].value_or(false)) {
            return;
        }

        char buffer[2048];
        std::vsnprintf(buffer, sizeof(buffer), _format, _argList);
        LOG(buffer);
    }

    void profilerBegin(
        const char *_name, uint32_t _abgr,
        const char *_filePath, uint16_t _line) override {}

    void profilerBeginLiteral(
        const char *_name, uint32_t _abgr,
        const char *_filePath, uint16_t _line) override {}

    void profilerEnd() override {}

    // TODO: should consider implementing caching at some point
    uint32_t cacheReadSize(uint64_t _id) override { return 0; }

    bool cacheRead(uint64_t _id, void *_data, uint32_t _size) override {
        return false;
    }

    void cacheWrite(uint64_t _id, const void *_data, uint32_t _size) override {}

    void screenShot(
        const char *_filePath, uint32_t _width, uint32_t _height,
        uint32_t _pitch, const void *_data,
        uint32_t _size, bool _yflip) override {}

    void captureBegin(
        uint32_t _width, uint32_t _height, uint32_t _pitch,
        bgfx::TextureFormat::Enum _format, bool _yflip) override {}

    void captureEnd() override {}

    void captureFrame(const void *_data, uint32_t _size) override {}
};

Renderer::Renderer() = default;

Renderer::~Renderer() {
    if (!this->initialized) {
        return;
    }

    for (auto *r : this->managed_resources) {
        r->destroy();
    }

    this->initialized = false;
}

void Renderer::init(uvec2 window_size) {
    // initialize bgfx
    bgfx::Init init;

    bgfx::PlatformData platform_data;
    global.platform->window->set_platform_data(platform_data);
    init.platformData = platform_data;

    this->callbacks = new Callbacks();
    init.callback = this->callbacks;

    init.type = bgfx::RendererType::Vulkan;
    init.resolution.width = window_size.x;
    init.resolution.height = window_size.y;
    init.resolution.reset = get_reset_flags(*this);

    if (!bgfx::init(init)) {
        ERROR("Error initializing BGFX");
        exit(1);
    }

    // mark initialized so renderer is properly destructed
    this->initialized = true;

    bgfx::reset(
        window_size.x, window_size.y,
        get_reset_flags(*this));
    bgfx::setDebug(BGFX_DEBUG_TEXT);

    this->capabilities = *bgfx::getCaps();

    // generate noise texture
    const auto noise_size = vec2(4, 4);
    auto rand = Rand(0x0153);
    auto noise_data = std::vector<vec4>(noise_size.x * noise_size.y);
    for (usize i = 0; i < noise_data.size(); i++) {
        noise_data[i] =
            vec4(
                rand.next(-1.0f, 1.0f),
                rand.next(-1.0f, 1.0f),
                rand.next(-1.0f, 1.0f),
                rand.next(-1.0f, 1.0f));
    }
    this->textures["noise"] = Texture(
        bgfx::createTexture2D(
            noise_size.x, noise_size.y, false, 1,
            bgfx::TextureFormat::RGBA32F,
            BGFX_SAMPLER_MIN_POINT
                | BGFX_SAMPLER_MAG_POINT
                | BGFX_SAMPLER_U_MIRROR
                | BGFX_SAMPLER_V_MIRROR,
            bgfx::copy(
                &noise_data[0],
                noise_data.size() * sizeof(noise_data[0]))),
        uvec2(noise_size));

    // generate palette texture
    constexpr auto PALETTE_SIZE = 32;
    auto palette_data = std::vector<u8>(
        PALETTE_SIZE * PALETTE_SIZE * PALETTE_SIZE * 4);
    for (usize r = 0; r < PALETTE_SIZE; r++) {
        for (usize g = 0; g < PALETTE_SIZE; g++) {
            for (usize b = 0; b < PALETTE_SIZE; b++) {
                const auto rgb = vec3(r, g, b) / vec3(PALETTE_SIZE - 1);
                const auto p = vec4(palette::find_nearest_color(rgb), 1.0f);
                const auto i =
                    4 *
                        ((b * PALETTE_SIZE * PALETTE_SIZE) +
                            (g * PALETTE_SIZE)
                                + r);

                *reinterpret_cast<u32*>(&palette_data[i]) =
                    PixelFormat::from_vec(
                        PixelFormat::DEFAULT,
                        p);
            }
        }
    }

    // TODO: better solution for 3D texture sizes
    this->textures["palette"] = Texture(
        bgfx::createTexture3D(
            PALETTE_SIZE, PALETTE_SIZE, PALETTE_SIZE,
            false,
            PixelFormat::to_texture_format(PixelFormat::DEFAULT),
            BGFX_SAMPLER_MIN_POINT
                | BGFX_SAMPLER_MAG_POINT
                | BGFX_SAMPLER_U_CLAMP
                | BGFX_SAMPLER_V_CLAMP,
            bgfx::copy(
                &palette_data[0], palette_data.size() * sizeof(u8))),
        uvec2(0));

    this->textures["sun_depth"] =
        make_buffer(
            uvec2(
                (*global.settings)["gfx"]["sun_depth_size"]
                    .value_or(1024)),
            bgfx::TextureFormat::D24F);

    // initialize vertex layouts
    VertexTypeManager::instance().init();

    this->sprite_renderer =
        std::make_unique<SpriteRenderer>();
    this->entity_highlighter =
        std::make_unique<EntityHighlighter>();
    this->debug =
        std::make_unique<DebugRenderer>(*this);
    this->mm_instancer =
        std::make_unique<MultiMeshInstancer>(&this->allocator);
}

void Renderer::init_with_resources() {
    this->particle_renderer = std::make_unique<ParticleRenderer>();
    this->font = std::make_unique<Font>();
    this->font->init();
    this->icon_renderer = std::make_unique<IconRenderer>();
}

void Renderer::prepare_frame() {
    bgfx::touch(VIEW_UI);
    bgfx::touch(VIEW_MAIN);
    bgfx::touch(VIEW_TRANSPARENT);
}

void Renderer::end_frame() {
    this->mm_instancer->upload();
    this->sprite_renderer->upload();
    this->frame_number = bgfx::frame();
    this->mm_instancer->frame();
    this->sprite_renderer->frame();
}

void Renderer::resize(uvec2 window_size) {
    this->window_size = window_size;
    this->target_size = get_render_size(*this, window_size);
    this->size = target_size + uvec2(2);

    LOG(
        "Display resized to {}/{}/{}",
        this->window_size, this->size, this->target_size);

    bgfx::reset(
        this->window_size.x, this->window_size.y,
        get_reset_flags(*this));
    bgfx::setViewRect(
        VIEW_MAIN, 0, 0,
        this->window_size.x, this->window_size.y);

    // TODO: pick best based on platform
    const auto
        format_depth = bgfx::TextureFormat::D24F,
        format_rgb8 = bgfx::TextureFormat::RGBA8,
        format_rgba8 = PixelFormat::to_texture_format(PixelFormat::DEFAULT),
        format_rgbaf = bgfx::TextureFormat::RGBA16F,
        format_rgba32 = bgfx::TextureFormat::RGBA32F,
        format_r32f = bgfx::TextureFormat::R32F;

    const auto buf_size = this->size;

    this->textures["composite"] = make_buffer(buf_size, format_rgb8);

    // GBUFFER 16-bit float/channel
    // RGB: color
    //   A: alpha
    this->textures["gbuffer"] =
        make_buffer(buf_size, format_rgbaf);
    this->textures["gbuffer_tr"] =
        make_buffer(buf_size, format_rgbaf);

    // NORMAL BUFFER 8 bits/channel
    // RGB: per-pixel normal
    //   A: shine ([0.0f, 1.0f] not encoded!)
    this->textures["normal"] =
        make_buffer(buf_size, format_rgba8);
    this->textures["normal_tr"] =
        make_buffer(buf_size, format_rgba8);

    // DATA BUFFER 32 bits/channel
    // R: flags (encoded u16)
    // G: unused
    // B: entity id (raw float)
    // A: tile id (raw float)
    this->textures["data"] =
        make_buffer(buf_size, format_rgba32);
    this->textures["data_tr"] =
        make_buffer(buf_size, format_rgba32);

    this->data_reader =
        std::make_unique<TextureReader>(
            this->textures["data"],
            VIEW_DATA_BLIT,
            format_rgba32,
            []() { return Renderer::get().frame_number; });

    // TODO: SSAO should be r8
    this->textures["depth"] = make_buffer(buf_size, format_depth);
    this->textures["depth_tr"] = make_buffer(buf_size, format_depth);
    this->textures["light"] = make_buffer(buf_size, format_rgb8);
    this->textures["light_tr"] = make_buffer(buf_size, format_rgb8);
    this->textures["bloom"] = make_buffer(buf_size, format_rgb8);
    this->textures["bloom_blur"] = make_buffer(buf_size, format_rgb8);
    this->textures["ssao"] = make_buffer(buf_size, format_rgb8);
    this->textures["ssao_blur"] = make_buffer(buf_size, format_rgb8);
    this->textures["blur0"] = make_buffer(buf_size, format_rgb8);
    this->textures["blur1"] = make_buffer(buf_size, format_rgb8);
    this->textures["ui"] = make_buffer(this->target_size, format_rgba8);

    // EDGE BUFFER
    // RGB: highlight for highlight entity IDs R/G/B
    // A: tile/depth edge
    this->textures["edge"] = make_buffer(buf_size, format_rgba8);

    // ENTITY STENCIL BUFFER
    // R: stores ID of stenciled entity as raw float
    this->textures["entity_stencil"] = make_buffer(buf_size, format_r32f);
    this->entity_stencil_reader =
        std::make_unique<TextureReader>(
            this->textures["entity_stencil"],
            VIEW_ENTITY_STENCIL_BLIT,
            format_r32f,
            []() { return Renderer::get().frame_number; });

    this->textures["ex"] = make_buffer(uvec2(256), format_rgba8);
    this->ex_reader =
        std::make_unique<TextureReader>(
            this->textures["ex"],
            VIEW_EX_BLIT,
            format_rgba8,
            []() { return Renderer::get().frame_number; });

    make_view(VIEW_MAIN, this->window_size);
    bgfx::setViewFrameBuffer(VIEW_MAIN, BGFX_INVALID_HANDLE);

    // COMPOSITE
    make_view(VIEW_COMPOSITE, buf_size);
    this->framebuffers["composite"] =
        make_framebuffer(
            VIEW_COMPOSITE,
            { Framebuffer::make_attachment(
                this->textures["composite"].handle) });

    // GBUFFER
    make_view(VIEW_GBUFFER, buf_size);
    this->framebuffers["gbuffer"] =
        make_framebuffer(
            VIEW_GBUFFER,
            {
                Framebuffer::make_attachment(
                    this->textures["gbuffer"].handle),
                Framebuffer::make_attachment(
                    this->textures["normal"].handle),
                Framebuffer::make_attachment(
                    this->textures["data"].handle),
                Framebuffer::make_attachment(
                    this->textures["depth"].handle)
            });

    // TRANSPARENT
    make_view(VIEW_TRANSPARENT, buf_size);
    this->framebuffers["transparent"] =
        make_framebuffer(
            VIEW_TRANSPARENT,
            {
                Framebuffer::make_attachment(
                    this->textures["gbuffer_tr"].handle),
                Framebuffer::make_attachment(
                    this->textures["normal_tr"].handle),
                Framebuffer::make_attachment(
                    this->textures["data_tr"].handle),
                Framebuffer::make_attachment(
                    this->textures["depth_tr"].handle)
            });

    // LIGHT
    make_view(VIEW_LIGHT, buf_size);
    this->framebuffers["light"] =
        make_framebuffer(
            VIEW_LIGHT,
            {
                Framebuffer::make_attachment(this->textures["light"].handle),
                Framebuffer::make_attachment(this->textures["bloom"].handle),
                Framebuffer::make_attachment(this->textures["light_tr"].handle)
            });

    // LIGHT_BLUR
    make_view(VIEW_BLOOM_BLUR, buf_size);
    this->framebuffers["bloom_blur"] =
        make_framebuffer(
            VIEW_BLOOM_BLUR,
            { Framebuffer::make_attachment(
                this->textures["bloom_blur"].handle) });

    // SUN
    make_view(VIEW_SUN, this->textures["sun_depth"].size);
    this->framebuffers["sun"] =
        make_framebuffer(
            VIEW_SUN,
            { Framebuffer::make_attachment(
                this->textures["sun_depth"].handle) });

    // SSAO
    make_view(VIEW_SSAO, buf_size);
    this->framebuffers["ssao"] =
        make_framebuffer(
            VIEW_SSAO,
            { Framebuffer::make_attachment(this->textures["ssao"].handle) });

    // SSAO_BLUR
    make_view(VIEW_SSAO_BLUR, buf_size);
    this->framebuffers["ssao_blur"] =
        make_framebuffer(
            VIEW_SSAO_BLUR,
            { Framebuffer::make_attachment(
                this->textures["ssao_blur"].handle) });

    // BLUR0
    make_view(VIEW_BLUR0, buf_size);
    this->framebuffers["blur0"] =
        make_framebuffer(
            VIEW_BLUR0,
            { Framebuffer::make_attachment(this->textures["blur0"].handle) });

    // BLUR1
    make_view(VIEW_BLUR1, buf_size);
    this->framebuffers["blur1"] =
        make_framebuffer(
            VIEW_BLUR1,
            { Framebuffer::make_attachment(this->textures["blur1"].handle) });


    // EDGE
    make_view(VIEW_EDGE, buf_size);
    this->framebuffers["edge"] =
        make_framebuffer(
            VIEW_EDGE,
            { Framebuffer::make_attachment(this->textures["edge"].handle) });

    // UI
    make_view(VIEW_UI, buf_size);
    this->framebuffers["ui"] =
        make_framebuffer(
            VIEW_UI,
            { Framebuffer::make_attachment(this->textures["ui"].handle) });

    // EX
    make_view(VIEW_EX, this->textures["ex"].size);
    this->framebuffers["ex"] =
        make_framebuffer(
            VIEW_EX,
            { Framebuffer::make_attachment(this->textures["ex"].handle) });

    // ENTITY_STENCIL
    make_view(VIEW_ENTITY_STENCIL, this->textures["entity_stencil"].size);
    this->framebuffers["entity_stencil"] =
        make_framebuffer(
            VIEW_ENTITY_STENCIL,
            { Framebuffer::make_attachment(
                    this->textures["entity_stencil"].handle) });
}

void Renderer::composite(
    const GameCamera &game_camera,
    RenderFn render,
    RenderFn render_ui,
    const Sun &sun,
    const OcclusionMap &occlusion_map,
    std::span<Light> lights) {
    // update texture readers
    this->data_reader->update();
    this->ex_reader->update();
    this->entity_stencil_reader->update();

    this->icon_renderer->update();

    const auto
        &texture = this->programs["texture"],
        &composite = this->programs["composite"],
        &composite_texture = this->programs["composite_texture"],
        &light = this->programs["light"],
        &ssao = this->programs["ssao"],
        &blur = this->programs["blur"],
        &edge = this->programs["edge"];

    this->entity_highlighter->set_uniforms(edge, composite);

    for (auto &[prefix, program] :
            types::make_array(
                std::make_tuple("u_look", &composite),
                std::make_tuple("u_look_light", &light),
                std::make_tuple("u_look_ssao", &ssao))) {
        StackAllocator<4096> allocator;
        UniformValueList us(&allocator);
        game_camera.camera.uniforms(us, prefix)
            .try_apply(*program);
    }

    auto screen_quad =
        [&](const Program &program,
            usize view,
            std::optional<RenderState> render_state = std::nullopt) {
            // screen-space camera
            const auto c = Camera(
                mat4(1.0),
                math::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f));
            c.set_view_transform(view);

            this->render_quad(
                vec3(-1.0f, -1.0f, 0.0f), vec3(1.0f, 1.0f, 0.0f),
                vec2(0.0f, 0.0f), vec2(1.0f, 1.0f),
                program,
                render_state ?
                    *render_state
                    : RenderState(view, 0, BGFX_STATE_WRITE_MASK));
        };

    // configure render order
    const auto order =
        types::make_array(
            VIEW_SUN,
            VIEW_GBUFFER,
            VIEW_TRANSPARENT,
            VIEW_LIGHT,
            VIEW_SSAO,
            VIEW_BLUR0,
            VIEW_BLUR1,
            VIEW_SSAO_BLUR,
            VIEW_BLOOM_BLUR,
            VIEW_EDGE,
            VIEW_COMPOSITE,
            VIEW_UI,
            VIEW_EX,
            VIEW_ENTITY_STENCIL,
            VIEW_DATA_BLIT,
            VIEW_EX_BLIT,
            VIEW_ENTITY_STENCIL_BLIT,
            VIEW_MAIN);
    bgfx::setViewOrder(0, order.size(), &order[0]);

    // render to sun's depth buffer
    sun.camera.set_view_transform(VIEW_SUN);
    render(
        RenderState(
            VIEW_SUN,
            RENDER_FLAG_PASS_SHADOW
                | RENDER_FLAG_PASS_FIRST,
            BGFX_STATE_WRITE_Z
                | BGFX_STATE_DEPTH_TEST_LESS
                | BGFX_STATE_CULL_CW));

    // render to deferred buffers
    game_camera.camera.set_view_transform(VIEW_GBUFFER);
    render(RenderState(VIEW_GBUFFER, RENDER_FLAG_PASS_BASE));

    // render to transparent deferred buffers
    game_camera.camera.set_view_transform(VIEW_TRANSPARENT);
    render(
        RenderState(
            VIEW_TRANSPARENT,
            RENDER_FLAG_PASS_TRANSPARENT,
            BGFX_STATE_WRITE_MASK
                | BGFX_STATE_DEPTH_TEST_ALWAYS
                | BGFX_STATE_CULL_CCW));

    // render current camera target entity into stencil buffer
    game_camera.camera.set_view_transform(VIEW_ENTITY_STENCIL);
    render(
        RenderState(
            VIEW_ENTITY_STENCIL,
            RENDER_FLAG_PASS_ENTITY_STENCIL
                | RENDER_FLAG_PASS_LAST,
            BGFX_STATE_WRITE_MASK
                | BGFX_STATE_DEPTH_TEST_ALWAYS
                | BGFX_STATE_CULL_CCW));

    // render to light buffer
    {
        StackAllocator<4096> allocator;
        UniformValueList us(&allocator);
        sun.uniforms(us).try_apply(light);
    }

    // upload lights
    Light::set_uniforms(light, lights);

    light.try_set("s_gbuffer", 0, this->textures["gbuffer"]);
    light.try_set("s_normal", 1, this->textures["normal"]);
    light.try_set("s_data", 2, this->textures["data"]);
    light.try_set("s_depth", 3, this->textures["depth"]);
    light.try_set("s_gbuffer_tr", 4, this->textures["gbuffer_tr"]);
    light.try_set("s_normal_tr", 5, this->textures["normal_tr"]);
    light.try_set("s_data_tr", 6, this->textures["data_tr"]);
    light.try_set("s_depth_tr", 7, this->textures["depth_tr"]);
    light.try_set("s_sun", 8, this->textures["sun_depth"]);
    light.try_set("s_noise", 9, this->textures["noise"]);
    occlusion_map.set_uniforms(light, 10, 11, 12);
    screen_quad(light, VIEW_LIGHT);

    // blur bloom buffer
    blur.try_set("s_input", 0, this->textures["bloom"]);
    blur.try_set("u_params", vec4(1, vec3(0)));
    screen_quad(blur, VIEW_BLUR0);

    blur.try_set("s_input", 0, this->textures["blur0"]);
    blur.try_set("u_params", vec4(0, vec3(0)));
    screen_quad(blur, VIEW_BLOOM_BLUR);

    // render to SSAO buffer
    // TODO: configurable count
    auto rand = Rand(0x5540);
    std::array<vec4, 64> ssao_samples;
    for (usize i = 0; i < ssao_samples.size(); i++) {
        const auto s =
            math::lerp(
                0.1f, 1.0f,
                pow(
                    i / static_cast<f32>(ssao_samples.size()),
                    2.0f));
        ssao_samples[i] =
            vec4(
                s * rand.next(0.0f, 1.0f) * math::normalize(
                    vec3(
                        rand.next<f32>(-1.0f, 1.0f),
                        rand.next<f32>(-1.0f, 1.0f),
                        rand.next<f32>(0.0f, 1.0f))),
                1.0);
    }

    ssao.try_set("ssao_samples", ssao_samples, ssao_samples.size());
    ssao.try_set("s_normal", 0, this->textures["normal"]);
    ssao.try_set("s_depth", 1, this->textures["depth"]);
    ssao.try_set("s_noise", 2, this->textures["noise"]);
    screen_quad(ssao, VIEW_SSAO);

    // blur ssao buffer
    blur.try_set("s_input", 0, this->textures["ssao"]);
    blur.try_set("u_params", vec4(1, vec3(0)));
    screen_quad(blur, VIEW_BLUR1);

    blur.try_set("s_input", 0, this->textures["blur1"]);
    blur.try_set("u_params", vec4(0, vec3(0)));
    screen_quad(blur, VIEW_SSAO_BLUR);

    // edge detection
    edge.try_set("s_input", 0, this->textures["depth"]);
    edge.try_set("s_data", 1, this->textures["data"]);
    screen_quad(edge, VIEW_EDGE);

    // composite
    composite.set("u_clear_color", this->clear_color);

    auto i_composite = 0;
    composite.try_set(
        "s_gbuffer", i_composite++, this->textures["gbuffer"]);
    composite.try_set(
        "s_normal", i_composite++, this->textures["normal"]);
    composite.try_set(
        "s_depth", i_composite++, this->textures["depth"]);
    composite.try_set(
        "s_sun", i_composite++, this->textures["sun_depth"]);
    /* composite.try_set( */
    /*     "s_noise", i_composite++, this->textures["noise"]); */
    composite.try_set(
        "s_entity_stencil", i_composite++, this->textures["entity_stencil"]);
    composite.try_set(
        "s_light", i_composite++, this->textures["light"]);
    composite.try_set(
        "s_light_tr", i_composite++, this->textures["light_tr"]);
    composite.try_set(
        "s_bloom", i_composite++, this->textures["bloom_blur"]);
    composite.try_set(
        "s_ssao", i_composite++, this->textures["ssao_blur"]);
    composite.try_set(
        "s_edge", i_composite++, this->textures["edge"]);
    composite.try_set(
        "s_data", i_composite++, this->textures["data"]);
    composite.try_set(
        "s_palette", i_composite++, this->textures["palette"]);
    composite.try_set(
        "s_gbuffer_tr", i_composite++, this->textures["gbuffer_tr"]);
    composite.try_set(
        "s_depth_tr", i_composite++, this->textures["depth_tr"]);
    occlusion_map.set_uniforms(
        composite,
        i_composite + 0,
        i_composite + 1,
        i_composite + 2);
    i_composite += 3;
    /* composite.try_set( */
    /*     "s_entity_stencil", i_composite++, this->textures["entity_stencil"]); */
    game_camera.set_uniforms(composite);

    screen_quad(composite, VIEW_COMPOSITE);

    // draw ui with alpha blending + no depth testing
    Camera(
        mat4(1.0),
        math::ortho(
            0.0f, f32(this->target_size.x),
            0.0f, f32(this->target_size.y),
            -1.0f, 1.0f))
        .set_view_transform(VIEW_UI);
    render_ui(
        RenderState(
            VIEW_UI,
            RENDER_FLAG_PASS_UI,
            BGFX_STATE_WRITE_RGB
                | BGFX_STATE_WRITE_A
                | BGFX_STATE_BLEND_ALPHA
                | BGFX_STATE_BLEND_FUNC(
                    BGFX_STATE_BLEND_SRC_ALPHA,
                    BGFX_STATE_BLEND_INV_SRC_ALPHA)));

    // draw to main
    composite_texture.try_set("s_tex", 0, this->textures["composite"]);
    game_camera.set_uniforms(composite_texture);
    screen_quad(composite_texture, VIEW_MAIN);

    // draw ui over with alpha blending
    texture.try_set("s_tex", 0, this->textures["ui"]);
    screen_quad(
        texture,
        VIEW_MAIN,
        RenderState(
            VIEW_MAIN,
            0,
            BGFX_STATE_WRITE_RGB
                | BGFX_STATE_WRITE_A
                | BGFX_STATE_BLEND_ALPHA
                | BGFX_STATE_BLEND_FUNC(
                    BGFX_STATE_BLEND_SRC_ALPHA,
                    BGFX_STATE_BLEND_INV_SRC_ALPHA)));
}

void Renderer::submit(
    const Program &program,
    const RenderState &render_state) const {
    program.try_set("u_render_flags", vec4(render_state.flags, vec3(0)));
    program.try_set("u_ticks", vec4(global.time->ticks));
    bgfx::setState(render_state.bgfx);
    bgfx::submit(render_state.view, program);
    program.reset();
}

void Renderer::render_quad(
    vec3 min, vec3 max,
    vec2 st_min, vec2 st_max,
    const Program &program,
    RenderState render_state) {
    const auto indices =
        std::array<u16, 6>{ 0, 1, 2, 0, 2, 3 };

    const auto vertices =
        types::make_array(
            VertexTexture(
                vec3(min.x, max.y, min.z),
                vec2(st_min.x, st_min.y)),
            VertexTexture(
                vec3(max.x, max.y, min.z),
                vec2(st_max.x, st_min.y)),
            VertexTexture(
                vec3(max.x, min.y, min.z),
                vec2(st_max.x, st_max.y)),
            VertexTexture(
                vec3(min.x, min.y, min.z),
                vec2(st_min.x, st_max.y)));

    this->render_buffers(
        program,
        mat4(1.0),
        VertexTexture::layout,
        std::span(std::as_const(vertices)),
        std::span(std::as_const(indices)),
        render_state);
}

void Renderer::render_aabb(
    const AABB &aabb,
    const vec4 &color,
    RenderState render_state) {

    std::array<VertexColorNormal, 24> vertices;
    std::array<u16, 36> indices;

    usize x = 0, v = 0;
    for (const auto &d : Direction::ALL) {
        const auto offset = v;
        for (usize i = 0; i < 4; i++) {
            const auto p =
                cube::VERTICES[
                    cube::INDICES[
                        (usize(d) * 6) + cube::UNIQUE_INDICES[i]]];
            vertices[v++] =
                VertexColorNormal(p, color, Direction::to_vec3(d));
        }

        for (const auto i : cube::FACE_INDICES) {
            indices[x++] = offset + i;
        }
    }

    const auto &program = this->programs["color"];
    const auto model =
        math::scale(
            math::translate(mat4(1.0f), aabb.min),
            aabb.size());
    program.try_set(
        "u_normal_mtx",
        math::normal_matrix(model));
    this->render_buffers(
        this->programs["color"],
        model,
        VertexColorNormal::layout,
        std::span(std::as_const(vertices)),
        std::span(std::as_const(indices)),
        render_state.or_defaults());
}
