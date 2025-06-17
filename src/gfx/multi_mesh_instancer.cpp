#include "gfx/multi_mesh_instancer.hpp"
#include "gfx/multi_mesh.hpp"
#include "constants.hpp"
#include "global.hpp"

void MultiMeshInstancer::frame_initialize() {
    if (this->frame_initialized) {
        return;
    }

    this->frame_initialized = true;
    this->max_instances_last_frame =
        this->max_instances;
    this->dummy_indices_size_last_frame =
        this->dummy_indices.size();

    // preallocate instance data buffer
    /* this->data_instance = */
    /*     global.frame_allocator.alloc_span<f32>( */
    /*         this->offset_instance_max); */
    this->data_instance.resize(0);
    this->offset_instance = 0;

    this->last_offset_data = this->offset_data;
}

void MultiMeshInstancer::upload() {
    if (this->max_instances > this->max_instances_last_frame) {
        // NOTE: dummy data *must* be F_CALLOC'd as otherwise the offset used to
        // calculate base_instance (i_data0.x) would potentially actually have
        // a value. and it *must* be used (otherwise BGFX complains!)
        const auto dummy =
            global.frame_allocator.alloc_span<vec4>(
                this->max_instances,
                Allocator::F_CALLOC);
        bgfx::update(
            this->buffer_instance_data,
            0,
            bgfx::makeRef(
                &dummy[0],
                dummy.size_bytes()));
    }

    // upload instance data
    if (this->data_instance.size() > 0) {
        bgfx::update(
            this->buffer_instance,
            0,
            bgfx::makeRef(
                &this->data_instance[0],
                this->data_instance.size() * sizeof(this->data_instance[0])));
    }

    // reupload mesh data buffer
    if (this->offset_data != this->last_offset_data) {
        LOG("ASDSASFASJFSAFJASF!!");
        bgfx::update(
            this->buffer_data,
            0,
            bgfx::makeRef(
                &this->data_mesh[0],
                this->data_mesh.size() * sizeof(this->data_mesh[0])));
    }

    // reupload dummy indices
    if (this->dummy_indices.size() >
            this->dummy_indices_size_last_frame) {
        bgfx::update(
            this->buffer_dummy_index,
            0,
            bgfx::makeRef(
                &this->dummy_indices[0],
                this->dummy_indices.size() * sizeof(this->dummy_indices[0])));

        bgfx::update(
            this->buffer_dummy_vertex,
            0,
            bgfx::makeRef(
                &this->dummy_vertices[0],
                this->dummy_vertices.size() * sizeof(this->dummy_vertices[0])));
    }
}

void MultiMeshInstancer::frame() {
    this->offset_instance_max =
        math::max(this->offset_instance, this->offset_instance_max);
    this->offset_instance = 0;
    this->frame_initialized = false;
}

static std::span<f32> ensure_span_capacity(std::span<f32> span, usize n) {
    if (span.size() >= n) {
        return span;
    }

    // TODO: add an allocator realloc_span??
    auto result = global.frame_allocator.alloc_span<f32>(n, Allocator::F_CALLOC);
    if (!span.empty()) {
        std::memcpy(&result[0], &span[0], span.size_bytes());
    }
    return result;
}

MultiMeshInstancer::RenderData MultiMeshInstancer::push(
    const MultiMeshInstanceBuffer &buffer) {
    ASSERT(buffer.mesh);
    return this->push(
        *buffer.mesh,
        std::span { buffer.data },
        std::span { buffer.meshes });
}

MultiMeshInstancer::RenderData MultiMeshInstancer::push(
    const MultiMesh &mesh,
    std::span<const ModelInstanceData> data,
    std::span<const MultiMeshEntry*> meshes) {
    // TODO: don't just do this in push, it is weird placement
    this->frame_initialize();

    MeshOffsets offsets;
    if (this->offsets.contains(&mesh)) {
        offsets = this->offsets.at(&mesh);
    } else {
        // TODO: don't rely on IDs, use mesh.num_entries() or something
        // total number of indices + padding
        const auto n_indices =
            mesh.num_entries() * mesh.n_indices_largest_entry;

        // size of data to be uploaded (in f32s)
        const auto data_size =
            n_indices
            + (mesh.vertex_data.size() / sizeof(f32));

        // resize backing buffer
        ASSERT(this->data_mesh.size() == this->offset_data);
        this->data_mesh.resize(this->data_mesh.size() + data_size);

        // upload mesh data into data buffer
        offsets.indices = this->offset_data;
        offsets.vertices = offsets.indices + n_indices;

        // write index data, padded and converted into f32s
        for (usize i = 0; i < mesh.num_entries(); i++) {
            const auto &entry = mesh.nth_entry(i);
            ASSERT(entry.id == i);

            // these entry indices are offsets into the data that is uploaded for
            // each entry in the multi mesh, NOT into the data for the entire
            // multi mesh
            //
            for (usize j = 0; j < mesh.n_indices_largest_entry; j++) {
                this->data_mesh[
                    offsets.indices
                        + (i * mesh.n_indices_largest_entry)
                        + j] =
                    j < entry.index_count ?
                        f32(entry.vertex_offset + mesh.index_data[entry.index_offset + j])
                        : f32(INDEX_INVALID);
            }
        }

        // write vertex data
        std::memcpy(
            &this->data_mesh[offsets.vertices],
            &mesh.vertex_data[0],
            mesh.vertex_data.size());

        // save offsets
        this->offsets[&mesh] = offsets;

        // extend offset by data size
        this->offset_data += data_size;
    }

    // ensure instance data capacity
    /* LOG("{}", this->offset_instance); */
    this->data_instance.resize(
        this->offset_instance
            + (data.size() * (sizeof(ModelInstanceData) / sizeof(f32)))
            + meshes.size() /* index_offset */);
    /* this->data_instance = */
    /*     ensure_span_capacity( */
    /*         this->data_instance, */
    /*         this->offset_instance */
    /*             + (data.size() * (sizeof(ModelInstanceData) / sizeof(f32))) */
    /*             + meshes.size() /1* index_offset *1/); */

    ASSERT(data.size() == meshes.size(), "data size != meshes size");

    const auto n_instances = data.size();
    this->max_instances = math::max(this->max_instances, n_instances);

    // copy instance data
    const auto base_offset_instance = this->offset_instance;
    for (usize i = 0; i < n_instances; i++) {
        std::memcpy(
            &this->data_instance[this->offset_instance],
            &data[i],
            sizeof(ModelInstanceData));
        this->offset_instance += sizeof(ModelInstanceData) / sizeof(f32);
        // does not use meshes[i]->index_offset because index data is padded
        // with zeroes to the largest entry
        this->data_instance[this->offset_instance] =
            meshes[i]->id * mesh.n_indices_largest_entry;
        /* LOG("{}/{}", meshes[i]->id, mesh.n_indices_largest_entry); */
        /* LOG("{}/{}/{}", */
        /*     fmt::ptr(&meshes[i]->generic_parent()), */
        /*     meshes[i]->id, */
        /*     mesh.n_indices_largest_entry); */
        this->offset_instance += 1;
    }

    // extend dummy indices if necessary
    if (mesh.n_indices_largest_entry > this->dummy_indices.size()) {
        this->dummy_indices.resize(mesh.n_indices_largest_entry);
        this->dummy_vertices.resize(mesh.n_indices_largest_entry);

        // set indices to gl_VertexID (index value)
        for (u16 i = 0; i < this->dummy_indices.size(); i++) {
            this->dummy_indices[i] = i;
        }

        // TODO: remove
        for (u16 i = 0; i < this->dummy_vertices.size(); i++) {
            this->dummy_vertices[i].position.x = i;
        }
    }

    return RenderData {
        .mesh = &mesh,
        .n_instances = n_instances,
        .offset_instance = base_offset_instance,
        .offset_indices = offsets.indices,
        .offset_vertices = offsets.vertices,
    };
}

void MultiMeshInstancer::render(
    const RenderData &render_data,
    RenderState render_state,
    const Program &program) const {
    // set buffers
    bgfx::setBuffer(
        BUFFER_INDEX_INSTANCE, this->buffer_instance, bgfx::Access::Read);
    bgfx::setBuffer(
        BUFFER_INDEX_INSTANCE_DATA, this->buffer_data, bgfx::Access::Read);

    // set instance data buffer, max instances are allocated in upload() if
    // necessary
    bgfx::setInstanceDataBuffer(
        this->buffer_instance_data,
        0,
        render_data.n_instances);

    program.set(
        "u_model_instance",
        vec4(
            render_data.offset_instance,
            render_data.offset_indices,
            render_data.offset_vertices,
            render_data.mesh->layout->getStride() / sizeof(f32)));

    mat4 model = mat4(1.0);
    bgfx::setTransform(math::value_ptr(model));
    bgfx::setVertexBuffer(
        0, this->buffer_dummy_vertex);
    bgfx::setIndexBuffer(
        this->buffer_dummy_index,
        0,
        render_data.mesh->n_indices_largest_entry);
    Renderer::get().submit(program, render_state);
}

void MultiMeshInstanceBuffer::push(
    std::span<const ModelInstanceData> data,
    std::span<const MultiMeshEntry*> meshes) {

    ASSERT(this->data.size() == this->meshes.size());
    ASSERT(data.size() == meshes.size());

    const auto n = this->data.size();

    this->data.resize(this->data.size() + data.size());
    std::memcpy(
        &this->data[n],
        &data[0],
        data.size_bytes());

    this->meshes.resize(this->meshes.size() + meshes.size());
    std::memcpy(
        &this->meshes[n],
        &meshes[0],
        meshes.size_bytes());
}
