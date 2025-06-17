#include "gfx/multi_mesh.hpp"
#include "gfx/renderer.hpp"
#include "global.hpp"

using namespace multi_mesh;

void multi_mesh::render(
    const MultiMesh &base,
    usize index_offset,
    usize index_count,
    usize vertex_offset,
    usize vertex_count,
    const Program &program,
    const mat4 &model,
    RenderState render_state) {
    ASSERT(bgfx::isValid(base.index_buffer));
    ASSERT(bgfx::isValid(base.vertex_buffer));

    if (program.has("u_normal_mtx")) {
        program.try_set("u_normal_mtx", math::normal_matrix(model));
    }

    bgfx::setTransform(math::value_ptr(model));
    bgfx::setIndexBuffer(
        base.index_buffer,
        index_offset, index_count);
    bgfx::setVertexBuffer(
        0, base.vertex_buffer,
        vertex_offset, vertex_count);
    Renderer::get().submit(program, render_state);
}

MultiMeshEntry multi_mesh::add(
    MultiMesh &base,
    usize n_vertices,
    std::span<const u8> vertices,
    std::span<const u16> indices) {
    ASSERT(bgfx::isValid(base.vertex_buffer));
    ASSERT(bgfx::isValid(base.index_buffer));

    // TODO: bad fix for bgfx bug
    // BGFX does not resize dynamic buffers when there is an offset into the
    // buffer given. this should be reported as a bug.

    // upload directly into CPU buffers
    base.index_data.resize(base.n_indices + indices.size());
    std::copy(
        indices.begin(), indices.end(),
        base.index_data.begin() + base.n_indices);

    const auto offset_v = base.vertex_data.size();
    base.vertex_data.resize(offset_v + vertices.size());
    std::copy(
        vertices.begin(), vertices.end(),
        base.vertex_data.begin() + offset_v);

    // TODO: FIX!!!!!
    // TODO: fix with lists
    // make sure that memory exists when uploading - cannot directly upload
    // vector memory as ref because otherwise memory could become invalidated
    // on resize
    const auto indices_p =
        global.frame_allocator.copy_span(base.index_data.span());
    bgfx::update(
        base.index_buffer, 0,
        bgfx::makeRef(&indices_p[0], indices_p.size_bytes()));

    const auto vertices_p =
        global.frame_allocator.copy_span(base.vertex_data.span());
    bgfx::update(
        base.vertex_buffer, 0,
        bgfx::makeRef(&vertices_p[0], vertices_p.size_bytes()));

    auto result = MultiMeshEntry {
        .mesh = &base,
        .id = base.next_entry_id++,
        .index_offset = base.n_indices,
        .index_count = indices.size(),
        .vertex_offset = base.n_vertices,
        .vertex_count = n_vertices,
    };

    base.n_indices += indices.size();
    base.n_vertices += n_vertices;

    base.n_indices_largest_entry =
        math::max<usize>(
            base.n_indices_largest_entry,
            indices.size());
    return result;
}
