#pragma once

#include "gfx/mesh_buffer.hpp"
#include "gfx/primitive.hpp"
#include "gfx/util.hpp"
#include "gfx/vertex.hpp"
#include "util/util.hpp"
#include "util/types.hpp"
#include "util/math.hpp"

struct Renderer;

struct DebugRenderer {
    RDResource<bgfx::DynamicVertexBufferHandle> vertex_buffer;
    RDResource<bgfx::DynamicIndexBufferHandle> index_buffer;

    explicit DebugRenderer(const Renderer &renderer);

    template <typename P>
        requires gfx::Primitive<P>
    void add(const P &p) {
        p.emit(
            this->buffers[
                p.is_transparent() ? PASS_TRANSPARENT : PASS_DEFAULT]);
    }

    void render(RenderState render_state) const;

    void reset();

private:
    enum Pass {
        PASS_DEFAULT = 0,
        PASS_TRANSPARENT = 1,
        PASS_COUNT = PASS_TRANSPARENT + 1
    };

    std::array<MeshBuffer<VertexColorNormal>, PASS_COUNT> buffers;

    const Renderer &renderer;
};
