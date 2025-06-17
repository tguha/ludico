#pragma once

#include "gfx/bgfx.hpp"
#include "gfx/util.hpp"
#include "util/resource.hpp"

struct SpriteBatch;
struct QuadBatch;

struct SpriteRenderer {
    RDResource<bgfx::IndexBufferHandle> index_buffer;
    RDResource<bgfx::VertexBufferHandle> vertex_buffer;
    RDResource<bgfx::DynamicIndexBufferHandle> instance_buffer;

    // current offset into instance buffer in bytes
    usize offset_instance = 0;

    // maximum offset reached in a frame, used to preallocate data buffer
    usize offset_max = 0;

    // instance data for this frame
    mutable std::span<u8> data_instance;

    SpriteRenderer();

    void render(
        const SpriteBatch &batch,
        RenderState render_state) const;

    void render(
        const QuadBatch &batch,
        RenderState render_state) const;

    // must be called before bgfx::frame
    void upload();

    // must be called once per frame, after bgfx::frame
    void frame();
};
