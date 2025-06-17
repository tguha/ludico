#pragma once

#include "util/allocator.hpp"
#include "util/bitset.hpp"
#include "util/nameof.hpp"
#include "util/log.hpp"
#include "util/assert.hpp"

template <usize I, usize J>
struct ElasticPoolAllocator;

// simple round-robin pool allocator for objects of size <= OBJECT_SIZE
// TODO: add configurable behavior specifying if allocation speed or locality
// ought to be prioritized (i.e. where do we start searching for the next free
// space?)
template <usize OBJECT_SIZE, usize N>
struct PoolAllocator : public Allocator {
    template <usize I, usize J>
    friend struct ElasticPoolAllocator;

    PoolAllocator() = default;
    PoolAllocator(const PoolAllocator&) = delete;
    PoolAllocator &operator=(const PoolAllocator&) = delete;

    PoolAllocator(PoolAllocator &&other) {
        *this = std::move(other);
    }

    PoolAllocator &operator=(PoolAllocator &&other) {
        this->next = other.next;
        this->bits = std::move(other.bits);
        this->buffer = other.buffer;
        this->dtors = std::move(other.dtors);
        this->allocator = other.allocator;

        other.buffer = nullptr;
        other.allocator = nullptr;
        return *this;
    }

    explicit PoolAllocator(Allocator *allocator)
        : Allocator(DP_NOTIFY),
          allocator(allocator) {}

    ~PoolAllocator() {
        if (this->allocator && this->buffer) {
            // run our remaining dtors
            for (usize i = 0; i < N; i++) {
                const auto &f = this->dtors[i];
                if (f) {
                    (*f)(&this->buffer[i * OBJECT_SIZE]);
                }
            }

            this->allocator->free(this->buffer);
        }
    }

    // true if p is in this pool's memory arena
    bool contains(void *p) const {
        return
            static_cast<uintptr_t>(reinterpret_cast<u8*>(p) - this->buffer)
                < (OBJECT_SIZE * N);
    }

    void *alloc_raw(usize n) override {
        ASSERT(
            n <= OBJECT_SIZE,
            "n={} is too large for pool {}",
            n,
            NAMEOF_TYPE(decltype(*this)));
        return alloc();
    }

    void free_raw(void *p) override {
        // run dtor, clear bit
        const auto i = this->index(p);
        if (auto &dtor = this->dtors[i]) {
            (*dtor)(p);
            dtor = std::nullopt;
        }
        this->bits.clear(i);
    }

    bool empty() const {
        return this->bits.popcnt() == 0;
    }

    bool full() const {
        return this->bits.popcnt() == N;
    }

protected:
    void add_dtor(void *p, DtorFn *f) override {
        this->dtors[this->index(p)] = f;
    }

    void free_dtor(void *p) override {
        // TODO: better error message
        ASSERT(false, "internal error");
    }

private:
    // get index of allocated pointer
    usize index(const void *p) const {
        const auto o = reinterpret_cast<const u8*>(p) - this->buffer;
        ASSERT(
            o % OBJECT_SIZE == 0,
            "misaligned index() on type {} for {}",
            NAMEOF_TYPE(decltype(*this)),
            fmt::ptr(p));

        return o / OBJECT_SIZE;
    }

    void *alloc() {
        ASSERT(
            this->allocator,
            "uninitialized pool allocator {} @ {}",
            NAMEOF_TYPE(decltype(*this)),
            fmt::ptr(this));

        if (!this->buffer) {
            this->buffer =
                reinterpret_cast<u8*>(
                    this->allocator->alloc(OBJECT_SIZE * N));
            ASSERT(this->buffer);
        }

        // TODO: replace with more intelligent seek
        usize n = 0;
        while (this->bits.get(this->next) && n != N) {
            this->next = (this->next + 1) % N;
            n++;
        }

        if (n == N) {
            WARN(
                "out of memory in pool {} at {}",
                NAMEOF_TYPE(decltype(*this)),
                fmt::ptr(this));
            return nullptr;
        }

        this->bits.set(this->next);
        this->next = (this->next + 1) % N;
        return &this->buffer[this->next * OBJECT_SIZE];
    }

    // next free slot
    usize next = 0;

    // 0 is free, 1 is used
    Bitset<N> bits = Bitset<N>();

    // memory buffer
    u8 *buffer = nullptr;

    // dtors
    std::array<std::optional<DtorFn*>, N> dtors;

    Allocator *allocator = nullptr;
};
