#include "gfx/vignette_renderer.hpp"
#include "gfx/renderer.hpp"
#include "gfx/vertex.hpp"
#include "global.hpp"

void VignetteRenderer::render(
    f32 intensity,
    const vec4 &color,
    RenderState render_state) {
    const auto &program = Renderer::get().programs["vignette"];
    program.set("u_color", color);
    program.set("u_intensity", vec4(intensity));

    const auto indices =
        std::array<u16, 6>{ 2, 1, 0, 3, 2, 0 };

    const auto scale = vec3(Renderer::get().size, 1.0f);
    const auto vertices =
        types::make_array(
            VertexTexture(
                vec3(0.0f, 0.0f, 0.0f) * scale,
                vec2(0.0f, 0.0f)),
            VertexTexture(
                vec3(1.0f, 0.0f, 0.0f) * scale,
                vec2(1.0f, 0.0f)),
            VertexTexture(
                vec3(1.0f, 1.0f, 0.0f) * scale,
                vec2(1.0f, 1.0f)),
            VertexTexture(
                vec3(0.0f, 1.0f, 0.0f) * scale,
                vec2(0.0f, 1.0f)));

    Renderer::get().render_buffers(
        program,
        mat4(1.0f),
        VertexTexture::layout,
        std::span(vertices),
        std::span(indices),
        render_state);
}
