#pragma once

#include "util/allocator.hpp"
#include "util/list.hpp"

template <usize N>
struct StackAllocator : public Allocator {
    StackAllocator()
        : Allocator(DP_INLINE | DP_NOTIFY),
          internal_allocator(this) {
        this->dtor_allocations =
           decltype(dtor_allocations)(&this->internal_allocator);
    }

    ~StackAllocator() {
        // run dtors for dtor allocations
        for (void *p : this->dtor_allocations) {
            DtorFn **p_dtor = this->inline_dtor(p);
            if (*p_dtor) {
                (*p_dtor)(p);
            }
        }
    }

    void *alloc_raw(usize n) override {
        const auto align =
            reinterpret_cast<usize>(this->i) % MAX_ALIGN != 0 ?
                MAX_ALIGN - (reinterpret_cast<usize>(this->i) % MAX_ALIGN)
                : 0;

        ASSERT(
            this->i + n + align < N,
            "{} at {} is out of space ({} + {})",
            NAMEOF_TYPE(decltype(*this)),
            fmt::ptr(this),
            this->i,
            n);
        void *p = &this->buffer[this->i + align];
        this->i += align + n;
        return p;
    }

    void free_raw(void*) override { /* no-op */ }

    void add_dtor(void *p, DtorFn*) override {
        this->dtor_allocations.push(p);
    }

private:
    // internal allocator which does not free
    struct InternalAllocator : public Allocator {
        StackAllocator *parent;

        InternalAllocator(StackAllocator *parent)
            : Allocator(DP_NOTHING),
              parent(parent) {}

        void *alloc_raw(usize n) override {
            return this->parent->alloc_raw(n);
        }

        void free_raw(void*) override {}
    } internal_allocator;

    BlockList<void*, 16> dtor_allocations;
    usize i = 0;
    u8 buffer[N];
};
