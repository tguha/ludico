#pragma once

#include "gfx/util.hpp"
#include "gfx/bgfx.hpp"
#include "gfx/uniform.hpp"
#include "gfx/program.hpp"
#include "gfx/texture.hpp"
#include "gfx/framebuffer.hpp"
#include "util/moveable.hpp"
#include "util/types.hpp"
#include "util/aabb.hpp"
#include "util/basic_allocator.hpp"
#include "util/util.hpp"

struct Light;
struct OcclusionMap;
struct GameCamera;
struct ParticleRenderer;
struct IconRenderer;
struct Font;
struct Sun;
struct IManagedResource;
struct TextureReader;
struct EntityHighlighter;
struct SpriteRenderer;
struct DebugRenderer;
struct MultiMeshInstancer;

struct Renderer {
    static constexpr bgfx::ViewId
        VIEW_MAIN           = 0,
        VIEW_COMPOSITE      = 1,
        VIEW_GBUFFER        = 2,
        VIEW_TRANSPARENT    = 3,
        VIEW_SUN            = 4,
        VIEW_LIGHT          = 5,
        VIEW_BLOOM_BLUR     = 6,
        VIEW_SSAO           = 7,
        VIEW_SSAO_BLUR      = 8,
        VIEW_BLUR0          = 9,
        VIEW_BLUR1          = 10,
        VIEW_EDGE           = 11,
        VIEW_UI             = 12,
        VIEW_EX             = 13,
        VIEW_ENTITY_STENCIL = 14,
        VIEW_EX_BLIT        = 15,
        VIEW_DATA_BLIT      = 16,
        VIEW_ENTITY_STENCIL_BLIT = 17;

    // uniforms, also stored by the programs they're used in
    std::unordered_map<
        u16,
        std::shared_ptr<Uniform>> uniforms;

    // programs
    std::unordered_map<std::string, Program> programs;

    // textures
    std::unordered_map<std::string, Texture> textures;

    // framebuffers
    std::unordered_map<std::string, Framebuffer> framebuffers;

    // arbitrary resources whose lifetime is attached to the renderer
    // pointers, as they should be initialized in their own translation units
    // and have static storage duration.
    // we also shouldn't run their destructors.
    std::vector<IManagedResource*> managed_resources;

    // texture readers
    std::unique_ptr<TextureReader>
        data_reader,
        ex_reader,
        entity_stencil_reader;

    // sprite renderer
    std::unique_ptr<SpriteRenderer> sprite_renderer;

    // particle renderer
    std::unique_ptr<ParticleRenderer> particle_renderer;

    // icon renderer
    std::unique_ptr<IconRenderer> icon_renderer;

    // font
    std::unique_ptr<Font> font;

    // entity highlighting
    std::unique_ptr<EntityHighlighter> entity_highlighter;

    // debug renderer
    std::unique_ptr<DebugRenderer> debug;

    // multi mesh instancer
    std::unique_ptr<MultiMeshInstancer> mm_instancer;

    // bgfx callbacks
    struct Callbacks;
    Callbacks *callbacks;

    // bgfx capabilities
    bgfx::Caps capabilities;

    // size of window backbuffer
    uvec2 window_size;

    // size of primary render target
    uvec2 size;

    // final render target size before backbuffer
    uvec2 target_size;

    // background clear color
    vec4 clear_color = vec4(0);

    // frame number as tracked by bgfx
    u32 frame_number = 0;

    u32 scale_factor = 5;

    // move-resetting boolean indicating if this renderer is
    // (1) initialized
    // (2) owns its managed resources
    Moveable<bool> initialized;

    // renderer-lifetime allocator
    BasicAllocator allocator;

    // get singleton renderer instance
    static Renderer &get();

    Renderer();
    Renderer(const Renderer&) = delete;
    Renderer(Renderer&&) = default;
    Renderer &operator=(const Renderer&) = delete;
    Renderer &operator=(Renderer&&) = default;
    ~Renderer();

    void init(uvec2 window_size);

    void init_with_resources();

    void prepare_frame();

    void end_frame();

    void resize(uvec2 window_size);

    void composite(
        const GameCamera &game_camera,
        RenderFn render,
        RenderFn render_ui,
        const Sun &sun,
        const OcclusionMap &occlusion_map,
        std::span<Light> lights);

    void submit(
        const Program &program,
        const RenderState &render_state) const;

    // renders whole vertex/index buffers using bgfx's transient buffers
    template<typename V, size_t N, typename I, size_t M>
    void render_buffers(
        const Program &program,
        const mat4 &model,
        const bgfx::VertexLayout &layout,
        std::span<const V, N> vertices,
        std::span<const I, M> indices,
        RenderState render_state) const {
        bgfx::TransientVertexBuffer vertex_buffer;
        bgfx::TransientIndexBuffer index_buffer;

        bool index32 = false;

        if constexpr (std::is_same_v<I, u32>) {
            index32 = true;
        }

        bgfx::allocTransientBuffers(
            &vertex_buffer, layout, vertices.size(),
            &index_buffer, indices.size(), index32);
        ASSERT(index_buffer.size >= sizeof(I) * indices.size());
        ASSERT(vertex_buffer.size >= sizeof(V) * vertices.size());

        std::memcpy(
            index_buffer.data, &indices[0],
            sizeof(I) * indices.size());

        std::memcpy(
            vertex_buffer.data, &vertices[0],
            sizeof(V) * vertices.size());

        bgfx::setTransform(math::value_ptr(model));
        bgfx::setVertexBuffer(0, &vertex_buffer);
        bgfx::setIndexBuffer(&index_buffer);
        this->submit(program, render_state);
    }

    // render directly from vertex/index buffers
    void render_buffers(
        bgfx::VertexBufferHandle vertex_buffer,
        bgfx::IndexBufferHandle index_buffer,
        const Program &program,
        const mat4 &model,
        RenderState render_state) {
        bgfx::setTransform(math::value_ptr(model));
        bgfx::setVertexBuffer(0, vertex_buffer);
        bgfx::setIndexBuffer(index_buffer);
        this->submit(program, render_state);
    }

    // basic primitive rendering functions
    void render_quad(
        vec3 min, vec3 max, vec2 st_min, vec2 st_max,
        const Program &program,
        RenderState render_state);

    void render_aabb(
        const AABB &aabb,
        const vec4 &color,
        RenderState render_state);
};
