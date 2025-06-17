#pragma once

#include "util/types.hpp"
#include "util/util.hpp"
#include "util/math.hpp"
#include "util/assert.hpp"

#include "gfx/bgfx.hpp"

// thin auto-deleting wrapper around bgfx::TextureHandle
// non-copyable
struct Texture {
    bgfx::TextureHandle handle = { bgfx::kInvalidHandle };
    uvec2 size;
    std::optional<u8*> data;

    Texture() = default;

    Texture(
        bgfx::TextureHandle handle,
        uvec2 size,
        std::optional<u8*> data = std::nullopt,
        bool has_ownership = true)
        : handle(handle),
          size(size),
          data(data),
          has_ownership(has_ownership) {}

    Texture(std::tuple<uvec2, bgfx::TextureHandle> t, bool has_ownership = true)
        : handle(std::get<1>(t)),
          size(std::get<0>(t)),
          has_ownership(has_ownership) {}

    Texture(const Texture &other) = delete;

    Texture(Texture &&other) {
        *this = std::move(other);
    }

    Texture &operator=(const Texture &other) = delete;

    Texture &operator=(Texture &&other) {
        this->handle = other.handle;
        this->size = other.size;
        this->has_ownership = other.has_ownership;
        this->data = other.data;
        other.handle = { bgfx::kInvalidHandle };
        other.has_ownership = false;
        return *this;
    }

    ~Texture() {
        if (this->has_ownership && bgfx::isValid(this->handle)) {
            LOG("Destroying texture {}", this->handle.idx);
            bgfx::destroy(handle);

            if (this->data) {
                std::free(*this->data);
            }
        }
    }

    inline operator bgfx::TextureHandle() const {
        return this->handle;
    }
private:
    bool has_ownership;
};
