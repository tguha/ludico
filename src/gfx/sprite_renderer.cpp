#include "gfx/sprite_renderer.hpp"
#include "gfx/renderer.hpp"
#include "gfx/quad_batch.hpp"
#include "gfx/sprite_batch.hpp"
#include "gfx/specular_textures.hpp"
#include "constants.hpp"
#include "global.hpp"

SpriteRenderer::SpriteRenderer() {
    const auto indices =
        std::array<u16, 6>{ 2, 1, 0, 3, 2, 0 };

    const auto vertices =
        types::make_array(
            VertexPosition(
                vec3(0.0f, 0.0f, 0.0f)),
            VertexPosition(
                vec3(1.0f, 0.0f, 0.0f)),
            VertexPosition(
                vec3(1.0f, 1.0f, 0.0f)),
            VertexPosition(
                vec3(0.0f, 1.0f, 0.0f)));

    auto [vb, ib] =
        gfx::create_buffers(std::span(vertices), std::span(indices));

    this->vertex_buffer = std::move(vb);
    this->index_buffer = std::move(ib);
    this->instance_buffer =
        gfx::as_bgfx_resource(
            bgfx::createDynamicIndexBuffer(
                1024,
                BGFX_BUFFER_ALLOW_RESIZE
                    | BGFX_BUFFER_COMPUTE_READ));
}

struct BufferData {
    union { vec4 pos_size; struct { vec2 pos; vec2 size; }; };
    vec4 color;
    vec4 alpha;
    union { vec4 stmm; struct { vec2 st_min; vec2 st_max; }; };

    BufferData() = default;

    BufferData(const SpriteBatch::Entry &e)
        : pos(e.pos),
          size(e.area.size()),
          color(e.color),
          alpha(e.alpha),
          st_min(e.area.min),
          st_max(e.area.max) {}

    BufferData(const QuadBatch::Entry &e)
        : pos(e.pos),
          size(e.size),
          color(e.color),
          alpha(1.0f),
          st_min(SpecularTextures::instance().max().min),
          st_max(SpecularTextures::instance().max().min) {}
} PACKED;

template <typename T>
static void render_data(
    SpriteRenderer &renderer,
    const List<T> &ts,
    RenderState render_state) {
    if (ts.empty()) {
        return;
    }

    // preallocate if no data yet
    // TODO: for some reason this solves a bug with single-frame position issues
    // with sprites - investigate further? (try removing the preallocation)
    if (renderer.data_instance.size() == 0) {
        renderer.data_instance =
            global.frame_allocator.alloc_span<u8>(
                renderer.offset_max);
    }

    // ensure capacity if data_instance span before upload
    const auto size_required =
        renderer.offset_instance + (ts.size() * sizeof(BufferData));
    if (renderer.data_instance.size_bytes() < size_required) {
        const auto old = renderer.data_instance;
        renderer.data_instance =
            global.frame_allocator.alloc_span<u8>(size_required);

        if (!old.empty()) {
            std::memcpy(
                &renderer.data_instance[0],
                &old[0],
                old.size_bytes());
        }
    }

    // convert into BufferData format
    auto data =
        global.frame_allocator.alloc_span<BufferData>(
            ts.size());
    std::copy(
        ts.begin(),
        ts.end(),
        data.begin());

    // upload after offset
    const auto offset = renderer.offset_instance;
    std::memcpy(
        &renderer.data_instance[offset],
        &data[0],
        data.size_bytes());
    renderer.offset_instance += data.size_bytes();

    bgfx::setBuffer(
        BUFFER_INDEX_SPRITE, renderer.instance_buffer, bgfx::Access::Read);

    // allocate dummy ID buffer
    bgfx::InstanceDataBuffer idb;
    bgfx::allocInstanceDataBuffer(&idb, ts.size(), sizeof(vec4));
    std::memset(idb.data, 0, idb.size);
    bgfx::setInstanceDataBuffer(&idb);
    ASSERT(idb.size == (ts.size() * sizeof(vec4)));

    const auto &program = Renderer::get().programs["sprite"];
    const auto offset_vec4s = offset / sizeof(vec4);
    program.set("u_offset", vec4(offset_vec4s));
    program.try_set("s_tex", 0, TextureAtlas::get());
    Renderer::get().render_buffers(
        renderer.vertex_buffer,
        renderer.index_buffer,
        program,
        mat4(1.0),
        render_state);
}

void SpriteRenderer::render(
    const SpriteBatch &batch,
    RenderState render_state) const {
    render_data(
        const_cast<SpriteRenderer&>(*this), batch.sprites, render_state);
}

void SpriteRenderer::render(
    const QuadBatch &batch,
    RenderState render_state) const {
    render_data(
        const_cast<SpriteRenderer&>(*this), batch.quads, render_state);
}

void SpriteRenderer::upload() {
    if (this->data_instance.empty()) {
        return;
    }

    bgfx::update(
        this->instance_buffer,
        0,
        bgfx::makeRef(
            &this->data_instance[0],
            this->data_instance.size_bytes()));
}

void SpriteRenderer::frame() {
    if (this->data_instance.empty()) {
        return;
    }

    this->data_instance = {};
    this->offset_max = math::max(this->offset_max, this->offset_instance);
    this->offset_instance = 0;
}
