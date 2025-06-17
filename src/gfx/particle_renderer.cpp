#include "gfx/particle_renderer.hpp"
#include "gfx/game_camera.hpp"
#include "gfx/vertex.hpp"
#include "gfx/util.hpp"
#include "gfx/renderer.hpp"
#include "level/level.hpp"
#include "constants.hpp"
#include "global.hpp"
#include "state/state_game.hpp"

ParticleRenderer::ParticleRenderer() {
    const auto indices =
        std::array<u16, 6>{ 0, 1, 2, 0, 2, 3 };

    const auto vertices =
        types::make_array(
            VertexPosition(
                vec3(0.0f, 0.0f, 0.0f) * vec3(1.0f / SCALE)),
            VertexPosition(
                vec3(0.0f, 1.0f, 0.0f) * vec3(1.0f / SCALE)),
            VertexPosition(
                vec3(1.0f, 1.0f, 0.0f) * vec3(1.0f / SCALE)),
            VertexPosition(
                vec3(1.0f, 0.0f, 0.0f) * vec3(1.0f / SCALE)));

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

    ASSERT(bgfx::isValid(this->vertex_buffer));
    ASSERT(bgfx::isValid(this->index_buffer));
    ASSERT(bgfx::isValid(this->instance_buffer));
}

void ParticleRenderer::enqueue(PixelParticle &&p) {
    this->pixel_particles.emplace_back(std::move(p));
}

void ParticleRenderer::flush(RenderState render_state) {
    if (this->pixel_particles.empty()) {
        return;
    }

    const bool is_transparent_pass =
        (render_state.flags & RENDER_FLAG_PASS_TRANSPARENT) != 0;

    render_state = render_state.or_defaults();
    render_state.bgfx &= ~(BGFX_STATE_CULL_CW | BGFX_STATE_CULL_CCW);

    auto data =
        global.frame_allocator.alloc_span<PixelParticle>(
            this->pixel_particles.size());
    const auto copy_end =
        std::copy_if(
            this->pixel_particles.begin(),
            this->pixel_particles.end(),
            data.begin(),
            [&](const auto &p) {
                return is_transparent_pass == (p.color.a != 1.0f);
            });
    data = data.subspan(0, copy_end - data.begin());

    // no pixels to render
    if (data.size() == 0) {
        return;
    }

    bgfx::update(
        this->instance_buffer, 0, bgfx::makeRef(&data[0], data.size_bytes()));
    bgfx::setBuffer(
        BUFFER_INDEX_PARTICLE, this->instance_buffer, bgfx::Access::Read);

    // TODO: consider pre-creating the ID buffer
    // create ID buffer
    bgfx::InstanceDataBuffer idb;
    bgfx::allocInstanceDataBuffer(
        &idb, this->pixel_particles.size(), sizeof(vec4));
    std::memset(idb.data, 0, idb.size);
    bgfx::setInstanceDataBuffer(&idb);
    ASSERT(idb.size == this->pixel_particles.size() * sizeof(vec4));

    const auto &program = Renderer::get().programs["particle"];
    Renderer::get().render_buffers(
        this->vertex_buffer,
        this->index_buffer,
        program,
        mat4(1.0),
        render_state);

    // reset particle buffer
    this->pixel_particles.resize(0);
}
