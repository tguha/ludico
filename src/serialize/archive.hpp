#pragma once

#include "util/types.hpp"
#include "util/util.hpp"
#include "util/math.hpp"

// archive/byte buffer
struct Archive {
    Archive() = default;
    Archive(std::span<u8> buffer)
        : _buffer(buffer),
          _position(0) {}

    virtual ~Archive() = default;

    void reset(usize position = 0) {
        this->_position = position;
    }

    usize position() const {
        return this->_position;
    }

    usize capacity() const {
        return this->_buffer.size();
    }

    usize skip(usize n) {
        const auto old = this->position();
        this->_position = math::min(this->capacity(), this->_position + n);
        return this->_position - old;
    }

    usize read(std::span<u8> dst) {
        const auto start = this->position();
        const auto n = this->skip(dst.size());
        std::memcpy(&dst[0], &this->_buffer[start], n);
        return n;
    }

    usize write(std::span<const u8> src) {
        if (this->_position + src.size() > this->capacity()) {
            this->on_size_exceeded(this->_position + src.size());
        }

        const auto start = this->position();
        const auto n = this->skip(src.size());
        std::memcpy(&this->_buffer[start], &src[0], n);
        return n;
    }

    std::span<u8> buffer() const {
        return this->_buffer;
    }

protected:
    // called when size of buffer will be exceeded by next write
    virtual void on_size_exceeded(usize exceeded_to) {}

    std::span<u8> _buffer;
    usize _position = 0;
};

// elastic archive which can expand on write
struct ElasticArchive : public Archive {
    ElasticArchive() = default;
    ElasticArchive(usize initial_capacity)
        : Archive(),
          _capacity(initial_capacity) {
        this->_bytes = reinterpret_cast<u8*>(
            std::malloc(this->_capacity));
        this->_buffer = std::span { this->_bytes, this->_capacity };
        this->_position = 0;
    }

    ~ElasticArchive() {
        if (this->_bytes) {
            std::free(this->_bytes);
        }
    }

    void on_size_exceeded(usize exceeded_to) override {
        while (this->_capacity < exceeded_to) {
            this->_capacity *= 2;
        }

        this->_bytes = reinterpret_cast<u8*>(
            std::realloc(this->_bytes, this->_capacity));
        this->_buffer = std::span { this->_bytes, this->_capacity };
    }

private:
    u8 *_bytes = nullptr;
    usize _capacity = 0;
};
