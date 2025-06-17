#pragma once

#include "util/types.hpp"
#include "util/util.hpp"
#include "util/math.hpp"
#include "util/resource.hpp"
#include "gfx/bgfx.hpp"
#include "gfx/texture_atlas.hpp"
#include "gfx/pixels.hpp"
#include "gfx/util.hpp"

struct Level;

struct ParticleRenderer {
    static constexpr auto MAX_PARTICLES = 1024;

    struct PixelParticle {
        union { struct { vec3 pos; float shine; }; vec4 _u0; };
        union { vec4 color; vec4 _u1; };

        PixelParticle() = default;
        PixelParticle(
            const vec3 &pos,
            const vec4 &color,
            float shine = 0.0f)
            : pos(pos),
              shine(shine),
              color(color) {}
    };

    RDResource<bgfx::VertexBufferHandle> vertex_buffer;
    RDResource<bgfx::IndexBufferHandle> index_buffer;
    RDResource<bgfx::DynamicIndexBufferHandle> instance_buffer;

    std::vector<PixelParticle> pixel_particles;

    ParticleRenderer();

    void enqueue(PixelParticle &&p);

    void flush(RenderState render_state);

    static inline auto colors_from_sprite(const TextureArea &area) {
        // retrieve pixel data
        const auto size = area.size();
        const auto pixels_size = size.x * size.y * 4;
        u8 pixels[pixels_size];
        area.parent->pixels({ pixels, pixels_size }, area);

        // get colors
        std::vector<vec3> colors;
        for (const auto &c : pixels::colors(Pixels(pixels, size))) {
            colors.emplace_back(c.rgb());
        }
        return colors;
    }
};
