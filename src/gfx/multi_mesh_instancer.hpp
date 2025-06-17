#pragma once

#include "util/resource.hpp"
#include "util/list.hpp"
#include "gfx/bgfx.hpp"
#include "gfx/renderer.hpp"
#include "gfx/vertex.hpp"
#include "gfx/util.hpp"

struct MultiMesh;
struct MultiMeshEntry;
struct MultiMeshInstanceBuffer;

// data for one instance of a model
struct ModelInstanceData {
    mat4 model;     // 4 * 4 = 16
    mat3 normal;    // 3 * 3 = 9
    f32 flags;      //       = 1
    f32 id;         //       = 1
                    //       = 27
};

static_assert(sizeof(ModelInstanceData) == 27 * sizeof(f32));


// TODO: more efficient multi mesh buffer space management - implement as LRU
// cache? at least don't re-upload every frame.
//
// TODO: only use one instance data buffer which gets consolidated for every
// frame?

// allows for drawing of multi meshes via instancing, wherein one multimesh +
// one group of instance data (models + normals + meshes) can be used to draw
// any mesh within the multimesh using THE SAME INSTANCE.
//
// works on a per-frame basis: data is gathered during each render() call,
// building a buffer for each MultiMesh which needs to be instanced for the
// frame. the vertex/index data for the mesh is sent to
// "buffer_data"/"data_data" and uploaded in upload() before the frame is
// rendered with bgfx::frame().
//
// the data is used by each instance buffer, refering to data in
// "buffer_instance" with a different offset for each render() call. this keeps
// the data passed to render() (models/normals/mesh offsets).
//
// the only downside of this approach is that, because of how the instancing
// works, N indices must be drawn for EACH instance where N = the number of
// indices for the largest entry in the multimesh. data is therefore padded such
// that, for example, if a mesh with 100 indices was drawn but the largest mesh
// in the multimesh has 128 indices then indices [100..127] will be set to
// INDEX_INVALID which will generate VERTEX_INVALID (see common.sc) in the
// vertex shader and therefore be discarded in the fragment shader
struct MultiMeshInstancer {
    struct RenderData {
        const MultiMesh *mesh;
        usize n_instances;
        usize offset_instance;
        usize offset_indices;
        usize offset_vertices;

        std::string to_string() const {
            return
                fmt::format(
                    "{}({}, {}, {}, {}, {})",
                    NAMEOF_TYPE(decltype(*this)),
                    fmt::ptr(mesh),
                    n_instances,
                    offset_instance,
                    offset_indices,
                    offset_vertices);
        }
    };

    // correspond to u_buffer_instance, u_buffer_data in vs_model_instanced
    RDResource<bgfx::DynamicIndexBufferHandle>
        buffer_instance, buffer_data;

    // TODO: top bound
    // dummy instance data buffer with size equal to largest number of instances
    RDResource<bgfx::DynamicVertexBufferHandle>
        buffer_instance_data;

    // dummy index buffer data
    RDResource<bgfx::DynamicIndexBufferHandle>
        buffer_dummy_index;

    // dummy vertex buffer data
    RDResource<bgfx::DynamicVertexBufferHandle>
        buffer_dummy_vertex;

    MultiMeshInstancer(
        Allocator *allocator,
        usize buffer_instance_size = 64 * 1024,
        usize buffer_data_size = 64 * 1024)
        : buffer_instance(
            gfx::as_bgfx_resource(
                bgfx::createDynamicIndexBuffer(
                    (buffer_instance_size * sizeof(f32)) / sizeof(u16),
                    BGFX_BUFFER_ALLOW_RESIZE
                        | BGFX_BUFFER_COMPUTE_READ
                        | BGFX_BUFFER_COMPUTE_TYPE_FLOAT))),
          buffer_data(
                gfx::as_bgfx_resource(
                    bgfx::createDynamicIndexBuffer(
                        (buffer_data_size * sizeof(f32)) / sizeof(u16),
                        BGFX_BUFFER_ALLOW_RESIZE
                            | BGFX_BUFFER_COMPUTE_READ
                            | BGFX_BUFFER_COMPUTE_TYPE_FLOAT))),
          buffer_instance_data(
                gfx::as_bgfx_resource(
                    bgfx::createDynamicVertexBuffer(
                        4096 * sizeof(vec4),
                        VertexDummy16::layout,
                        BGFX_BUFFER_ALLOW_RESIZE
                        // TODO: do we need compute read?
                            | BGFX_BUFFER_COMPUTE_READ))),
          buffer_dummy_index(
                gfx::as_bgfx_resource(
                    bgfx::createDynamicIndexBuffer(
                        4096,
                        BGFX_BUFFER_ALLOW_RESIZE))),
          buffer_dummy_vertex(
                gfx::as_bgfx_resource(
                    bgfx::createDynamicVertexBuffer(
                        4096,
                        VertexPosition::layout,
                        BGFX_BUFFER_ALLOW_RESIZE))) {}
          // TODO: lists , dummy_indices(allocator) {}

    // TODO: doc
    RenderData push(
        const MultiMeshInstanceBuffer &buffer);

    // TODO: doc
    // TODO: convert meshes to array of refs? not sure
    RenderData push(
        const MultiMesh &mesh,
        std::span<const ModelInstanceData> data,
        std::span<const MultiMeshEntry*> meshes);

    void render(
        const RenderData &render_data,
        RenderState render_state,
        const Program &program) const;

    // must be called before bgfx::frame
    void upload();

    // must be called once per frame, after bgfx::frame
    void frame();
// TODO: private:
    // mesh offsets into data_data in f32s
    struct MeshOffsets {
        usize indices, vertices;
    };

    // true if buffers have been intialized for this frame
    bool frame_initialized = false;

    // intializes buffers for this frame if not already done
    void frame_initialize();

    // backing frame allocated buffer for frame instance data
    //std::span<f32> data_instance;
    std::vector<f32> data_instance;

    // backing buffer for all mesh data
    std::vector<f32> data_mesh;

    // maximum number of instances used for one render(), current and last frame
    usize max_instances_last_frame = 0, max_instances = 0;

    // size of dummy indices last frame
    usize dummy_indices_size_last_frame = 0;

    // TODO: set a reasonable top limit (?)
    // list of dummy indices where list[i] = i to be provided when rendering
    // TODO: lists List<u16> dummy_indices;
    std::vector<u16> dummy_indices;

    // TODO: remove
    std::vector<VertexPosition> dummy_vertices;

    // offset into instance data (cleared per-frame)
    usize offset_instance = 0;

    // maximum data offset reached
    // (used to pre-allocating instance data buffer)
    usize offset_instance_max = 0;

    // offset into data
    usize offset_data = 0;

    // offset into data from last frame
    usize last_offset_data = 0;

    // stored per-frame offsets of MultiMeshes into data, instance buffers
    std::unordered_map<const MultiMesh*, MeshOffsets> offsets;
};

// TODO: doc
struct MultiMeshInstanceBuffer {
    Allocator *allocator;
    const MultiMesh *mesh;
    mutable std::vector<ModelInstanceData> data;
    mutable std::vector<const MultiMeshEntry*> meshes;

    // TODO: lists mutable List<ModelInstanceData> data;
    // TODO: lists mutable List<const MultiMeshEntry*> meshes;

    MultiMeshInstanceBuffer() = default;

    MultiMeshInstanceBuffer(
        Allocator *allocator,
        const MultiMesh &mesh)
        : allocator(allocator),
          mesh(&mesh){}
          // TODO: lists data(allocator),
          // TODO: lists meshes(allocator) {}

    void push(
        std::span<const ModelInstanceData> data,
        std::span<const MultiMeshEntry*> meshes);

    inline void push(
        const ModelInstanceData &m,
        const MultiMeshEntry *e) {
        this->push(std::span { &m, 1 }, std::span { &e, 1 });
    }

    inline void clear() {
        this->data.clear();
        this->meshes.clear();
    }
};
