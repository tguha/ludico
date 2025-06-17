#pragma once

#include "util/util.hpp"
#include "util/types.hpp"
#include "util/assert.hpp"
#include "util/allocator.hpp"
#include "util/tracker.hpp"
#include "util/list.hpp"

struct BumpAllocator : public Allocator {
    struct Block {
        Allocator *allocator;
        usize size;
        u8 *mem, *cur, *end;
        Block *next;

        Block(const Block &other) = delete;
        Block(Block &&other) { *this = std::move(other); }
        Block &operator=(const Block &other) = delete;
        Block &operator=(Block &&other) {
            this->mem = other.mem;
            this->cur = other.cur;
            this->end = other.end;
            this->next = other.next;
            other.mem = nullptr;
            other.next = nullptr;
            return *this;
        }

        explicit Block(Allocator *allocator, usize size)
            : allocator(allocator),
              size(size) {
            this->mem =
                reinterpret_cast<u8*>(
                    this->allocator->alloc(size));
            this->cur = this->mem;
            this->end = this->mem + size;
            this->next = nullptr;
        }

        ~Block() {
            if (this->mem) {
                this->allocator->free(this->mem);
                this->mem = nullptr;
            }
        }

        inline void *alloc(usize n) {
            ASSERT(n <= this->size);
            ASSERT(this->mem, "Bump allocator not initialized");

            usize align =
                reinterpret_cast<usize>(this->cur) % MAX_ALIGN != 0 ?
                    (MAX_ALIGN
                        - (reinterpret_cast<usize>(this->cur) % MAX_ALIGN))
                    : 0;

            if (this->cur + align + n >= this->end) {
                return nullptr;
            }

            void *res = this->cur + align;
            this->cur += align + n;
            return res;
        }

        inline void clear() {
            this->cur = this->mem;
        }
    };

    struct Dtor {
        void *p;
        DtorFn *f;
    };

    Allocator *allocator = nullptr;
    usize size = 0, block_size = 0, allocated = 0;
    Block *block = nullptr;
    BlockList<Dtor, 16> dtors;

    BumpAllocator() = default;
    BumpAllocator(const BumpAllocator &other) = delete;
    BumpAllocator(BumpAllocator &&other) { *this = std::move(other); }
    BumpAllocator &operator=(const BumpAllocator &other) = delete;
    BumpAllocator &operator=(BumpAllocator &&other) {
        if (other.block) {
            this->block = other.block;
            other.block = nullptr;
        } else {
            this->block = nullptr;
        }

        this->allocator = other.allocator;
        this->size = other.size;
        this->block_size = other.block_size;
        this->dtors = std::move(other.dtors);

        return *this;
    }

    explicit BumpAllocator(Allocator *allocator, usize block_size)
        : Allocator(DP_NOTIFY) {
        this->allocator = allocator;
        this->dtors = decltype(this->dtors)(this->allocator);
        this->block = nullptr;
        this->size = block_size;
        this->block_size = block_size;
    }

    ~BumpAllocator() {
        Block *b = this->block;
        while (b) {
            Block *old = b;
            b = b->next;
            this->allocator->free(old);
        }
    }

    void free_raw(void*) override { /* no-op */ }

    void *alloc_raw(usize n) override {
        ASSERT(
            n <= this->block_size,
            "Allocation of size " + std::to_string(n)
                + " too big (block size "
                + std::to_string(this->block_size) + ")");

        this->allocated += n;

        Block *block = this->block, *last = nullptr;
        while (block != nullptr) {
            void *result = block->alloc(n);

            if (result) {
                return result;
            }

            last = block;
            block = block->next;
        }

        // could not allocate, expand
        LOG(
            "Allocating new block of size {} (allocator @ {})",
            this->block_size, fmt::ptr(this));

        Block *b =
            this->allocator->alloc<Block>(this->allocator, this->block_size);
        ASSERT(b);

        if (last) {
            last->next = b;
        } else {
            this->block = b;
        }

        void *result = b->alloc(n);
        ASSERT(result);
        return result;
    }

    inline void clear() {
        if (!this->block) {
            return;
        }

        // run destructors
        for (auto &d : this->dtors) {
            d.f(d.p);
        }

        this->dtors.clear();

        usize total = 0;

        Block *block = this->block;
        while (block != nullptr) {
            total += block->size;
            block->clear();
            block = block->next;
        }

        // only allow size increases
        this->size = std::max(this->size, total);

        // reallocate into one mega block
        if (this->size > this->block->size) {
            this->allocator->free(this->block);
            this->block =
                this->allocator->alloc<Block>(this->allocator, this->size);
            LOG(
                "Allocating new mega block of size {} (allocator @ {})",
                this->size, fmt::ptr(this));
        }

        this->allocated = 0;
    }

protected:
    void add_dtor(void *p, DtorFn *f) override {
        this->dtors.allocator = this;
        this->dtors.push(Dtor { p, f });
    }

    void free_dtor(void *p) override { /* no-op */ }
};

// bump allocator which only gets cleared after n consecutive frames where
// its allocated amount does not change
// use for bgfx::* calls which take a memory, as these might need the
// memory to exist for multiple frames
template <usize N>
struct LongAllocator : public BumpAllocator {
    using BumpAllocator::BumpAllocator;

    void frame() {
        if (this->allocated == this->last_allocated) {
            this->frames_since_change++;
        }

        this->last_allocated = this->allocated;

        if (this->frames_since_change > N) {
            this->clear();
        }
    }
private:
    usize frames_since_change = 0, last_allocated = 0;
};
