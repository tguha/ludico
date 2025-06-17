#include "gfx/mesh.hpp"
#include "gfx/renderer.hpp"
#include "global.hpp"

_MESH_EXTERN_TEMPLATE_I_E(u16,)
_MESH_EXTERN_TEMPLATE_I_E(u32,)

void Mesh::render(
    const Program &program,
    const mat4 &model,
    RenderState render_state) const {
    ASSERT(bgfx::isValid(this->index_buffer));
    ASSERT(bgfx::isValid(this->vertex_buffer));
    program.try_set("u_normal_mtx", math::normal_matrix(model));
    bgfx::setTransform(math::value_ptr(model));
    bgfx::setIndexBuffer(this->index_buffer);
    bgfx::setVertexBuffer(0, this->vertex_buffer);
    Renderer::get().submit(program, render_state);
}
