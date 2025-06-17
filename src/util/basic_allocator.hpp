#pragma once

#include "util/allocator.hpp"

struct BasicAllocator : public Allocator {
    usize n_unfreed = 0;

    BasicAllocator()
        : Allocator(DP_INLINE) {}

    ~BasicAllocator() {
        if (this->n_unfreed > 0) {
            WARN(
                "{} @ {} has {} unfree'd allocations",
                NAMEOF_TYPE(decltype(*this)),
                fmt::ptr(this),
                this->n_unfreed);
        }
    }

    void *alloc_raw(usize n) override {
        this->n_unfreed++;
        return std::malloc(n);
    }

    void free_raw(void *p) override {
        this->n_unfreed--;
        std::free(p);
    }
};
