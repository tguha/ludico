#pragma once

#include "util/types.hpp"
#include "util/util.hpp"
#include "util/log.hpp"
#include "gfx/bgfx.hpp"

// thin auto-deleting wrapper around bgfx::FramebufferHandle
// non-copyable
struct Framebuffer {
    bgfx::FrameBufferHandle handle = { bgfx::kInvalidHandle };

    Framebuffer() = default;

    Framebuffer(bgfx::FrameBufferHandle handle) : handle(handle) {}

    Framebuffer(const Framebuffer &other) = delete;

    Framebuffer(Framebuffer &&other) {
        *this = std::move(other);
    }

    Framebuffer &operator=(const Framebuffer &other) = delete;

    Framebuffer &operator=(Framebuffer &&other) {
        this->handle = other.handle;
        other.handle = { bgfx::kInvalidHandle };
        return *this;
    }

    ~Framebuffer() {
        if (this->handle.idx != bgfx::kInvalidHandle) {
            LOG("Destroying framebuffer {}", this->handle.idx);
            bgfx::destroy(this->handle);
        }
    }

    inline operator bgfx::FrameBufferHandle() {
        return this->handle;
    }

    template<typename... Args>
    static inline auto make_attachment(Args... args) {
        bgfx::Attachment attachment;
        attachment.init(args...);
        return attachment;
    }
};
