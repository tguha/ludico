#pragma once

#include "util/resource.hpp"
#include "util/assert.hpp"
#include "gfx/util.hpp"
#include "gfx/bgfx.hpp"

struct Model {
    Model() = default;
    Model(const Model &other) = delete;
    Model(Model &&other) = default;
    Model &operator=(const Model &other) = delete;
    Model &operator=(Model &&other) = default;
};

// TODO: fundamentally broken, dynamic index buffers are bad. replace with
// something else?
// TODO: can be removed
struct ModelInstanceBuffer {
    struct InstanceData {
        const ModelInstanceBuffer *parent;
        usize offset;
        bgfx::InstanceDataBuffer idb;
    };

    RDResource<bgfx::DynamicIndexBufferHandle> buffer;
    usize size = 0, offset = 0;

    ModelInstanceBuffer() = default;
    ModelInstanceBuffer(usize size)
        : size(size) {
        this->buffer = gfx::as_bgfx_resource(
            bgfx::createDynamicIndexBuffer(
                size / sizeof(u16),
                BGFX_BUFFER_COMPUTE_READ
                    | BGFX_BUFFER_ALLOW_RESIZE));
    }

    inline operator bgfx::DynamicIndexBufferHandle() const {
        return this->buffer;
    }

    inline void reset() {
        this->offset = 0;
    }

    template <typename T>
    inline InstanceData upload(
        usize n_instances, T *data, usize n, bool copy = false) {
        if (this->offset + n >= this->size) {
            WARN(
                "No more space in model instance buffer @ {} ({} bytes)",
                fmt::ptr(this), this->size);
            return InstanceData { this, 0, {} };
        }

        const auto offset = this->offset;
        bgfx::update(
            this->buffer, this->offset / sizeof(u16),
            copy ? bgfx::copy(data, n) : bgfx::makeRef(data, n));
        this->offset += n;

        bgfx::InstanceDataBuffer idb;
        bgfx::allocInstanceDataBuffer(&idb, n_instances, 16);
        ASSERT(idb.size == n_instances * 16);
        return InstanceData { this, offset, idb };
    }
};
