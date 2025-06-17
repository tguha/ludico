#pragma once

#include "util/allocator.hpp"
#include "util/bitset.hpp"
#include "util/nameof.hpp"
#include "util/log.hpp"
#include "util/assert.hpp"
#include "util/pool_allocator.hpp"

// elastic pool allocator for objects of size <= OBJECT_SIZE with blocks of size
// BLOCK_SIZE
//
// NOTE: ElasticPoolAllocator does not need to do anything about dtors as it
// does not actually allocate anything (all allocation is deferred to the pools
// inside of blocks) so all dtors are done either from the base allocator (when
// blocks are free'd) or on the blocks (pool allocators) themselves
//
// TODO: remember last block with free space
template <usize OBJECT_SIZE, usize BLOCK_SIZE>
struct ElasticPoolAllocator : public Allocator {
private:
    struct Block {
        PoolAllocator<OBJECT_SIZE, BLOCK_SIZE> pool;
        Block *next;

        Block() = default;
        Block(
            PoolAllocator<OBJECT_SIZE, BLOCK_SIZE> &&pool,
            Block *next)
            : pool(std::move(pool)),
              next(next) {}
    };

    // first block
    Block *block = nullptr;

    // base allocator
    Allocator *allocator = nullptr;
public:

    ElasticPoolAllocator() = default;
    ElasticPoolAllocator(const ElasticPoolAllocator&) = delete;
    ElasticPoolAllocator &operator=(const ElasticPoolAllocator&) = delete;

    ElasticPoolAllocator(ElasticPoolAllocator &&other) {
        *this = std::move(other);
    }

    ElasticPoolAllocator &operator=(ElasticPoolAllocator &&other) {
        this->block = other.block;
        this->allocator = other.allocator;
        other.block = nullptr;
        other.allocator = nullptr;
        return *this;
    }

    explicit ElasticPoolAllocator(Allocator *allocator)
        : Allocator(DP_NOTHING),
          allocator(allocator) {}

    ~ElasticPoolAllocator() {
        if (!this->allocator) {
            return;
        }

        Block *b = this->block;
        while (b) {
            Block *next = b->next;
            this->allocator->free(b);
            b = next;
        }
    }

    void *alloc_raw(usize n) override {
        ASSERT(
            n <= OBJECT_SIZE,
            "n={} is too large for pool {}",
            n,
            NAMEOF_TYPE(decltype(*this)));
        return this->alloc();
    }

    void free_raw(void *p) override {
        Block *b = this->block_of(p);
        ASSERT(b, "no block which contains {}", fmt::ptr(p));
        b->pool.free(p);
    }

private:
    // find block which contains p, returning nullptr if there is no such block
    Block *block_of(void *p) const {
        Block *b = this->block;
        while (b && !b->pool.contains(p)) {
            b = b->next;
        }
        return b;
    }

    // get current block with free memory, creating one if not found
    Block *current() {
        Block *b = this->block, *last = nullptr;
        while (b && b->pool.full()) {
            last = b;
            b = b->next;
        }

        if (!b) {
            // create new block
            b =
                this->allocator->template alloc<Block>(
                    PoolAllocator<OBJECT_SIZE, BLOCK_SIZE>(this->allocator),
                    nullptr);

            if (last) {
                last->next = b;
            } else {
                this->block = b;
            }
        }

        ASSERT(!b->pool.full());
        return b;
    }

    // allocate an object
    void *alloc() {
        return this->current()->pool.alloc();
    }
};
