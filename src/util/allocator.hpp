#pragma once

#include "util/util.hpp"
#include "util/types.hpp"
#include "util/assert.hpp"

// TODO: platform specific - but for some reason alignof(std::max_align_t) gives
//       8 when some pointers expect 16...
#define MAX_ALIGN 16

// TODO: convert into Allocator (generic non-templated interface) and
//       TAllocator<...> which contains some information about allocator
//       policies (dtor policy, does "free" actually do anything, etc.)

struct Allocator {
    // destructor function: return value is pointer to originally allocated
    // memory
    using DtorFn = void*(void*);

    // size of offset down to pointer to inline destructors
    static constexpr auto INLINE_DTOR_OFFSET = MAX_ALIGN;

    enum DtorPolicy : u8 {
        // do nothing about destructors
        DP_NOTHING = 0,

        // allocate an extra sizeof(void*) space for each allocation which is
        // either:
        // * nullptr if T does not have a destructor
        // * void(*)(void*) if T has a destructor
        DP_INLINE  = (1 << 0),

        // calls:
        // * add_dtor(void *p) AFTER some T has been allocated at p
        // * free_dtor(void *p) when free(p) is called
        DP_NOTIFY  = (1 << 1)
    };

    // allocation flags
    enum Flags : u8 {
        F_NONE      = 0,
        F_CALLOC    = (1 << 0)
    };

    Allocator(u8 dtor_policy = DP_NOTHING)
        : dtor_policy(dtor_policy) {}

    virtual ~Allocator() = default;

    void *alloc(usize n, Flags flags = F_NONE) {
        void *p = nullptr;

        if (this->dtor_policy & DP_INLINE) {
            // write nullptr inline dtor for raw allocations
            void *ptr = this->alloc_raw(INLINE_DTOR_OFFSET + n);
            *reinterpret_cast<DtorFn**>(ptr) = nullptr;
            p = reinterpret_cast<char*>(ptr) + INLINE_DTOR_OFFSET;
        } else {
            p = this->alloc_raw(n);
        }

        if (flags & F_CALLOC) {
            std::memset(p, 0, n);
        }

        return p;
    }

    // free a pointer which has previously been returned by some alloc* variant
    void free(void *p) {
        void *q = p;

        if (this->dtor_policy & DP_NOTIFY) {
            this->free_dtor(p);
        }

        if (this->dtor_policy & DP_INLINE) {
            DtorFn **p_dtor = this->inline_dtor(p);
            if (*p_dtor) {
                q = (*p_dtor)(p);
            } else {
                // unless dtor says otherwise (like for arrays which store
                // their count first), p_dtor is where the actual allocation
                // occured
                q = p_dtor;
            }
        }

        this->free_raw(q);
    }

    template <typename T, typename ...Args>
    T *alloc(Args&& ...args) {
        T *ptr;
        DtorFn *dtor = nullptr;

        if constexpr (std::is_trivially_destructible_v<T>) {
            dtor = nullptr;
        } else {
            if (this->dtor_policy & DP_INLINE) {
                dtor =
                    [](void *p) -> void* {
                        reinterpret_cast<T*>(p)->~T();
                        return reinterpret_cast<char*>(p) - INLINE_DTOR_OFFSET;
                    };
            } else {
                dtor =
                    [](void *p) -> void* {
                        reinterpret_cast<T*>(p)->~T();
                        return p;
                    };
            }
        }

        if (this->dtor_policy & DP_INLINE) {
            void *p =
                this->alloc_raw(INLINE_DTOR_OFFSET + sizeof(T));
            ASSERT(p);

            *reinterpret_cast<DtorFn**>(p) = dtor;
            ptr =
                reinterpret_cast<T*>(
                    reinterpret_cast<char*>(p) + INLINE_DTOR_OFFSET);
        } else {
            ptr = reinterpret_cast<T*>(this->alloc_raw(sizeof(T)));
            ASSERT(ptr);
        }

        if ((this->dtor_policy & DP_NOTIFY) && dtor) {
            this->add_dtor(ptr, dtor);
        }

        new (ptr) T(std::forward<Args>(args)...);
        return ptr;
    }

    template <typename T>
    T *alloc_array(usize size, Flags flags = F_NONE) {
        ASSERT(
            size != 0,
            "attempt to allocate array {}[] of size 0",
            NAMEOF_TYPE(T));

        T *ts = nullptr;
        DtorFn **p_dtor = nullptr;
        usize *p_size = nullptr;

        if constexpr (std::is_trivially_destructible_v<T>) {
            // still need to allocate an (empty!) dtor ptr if policy says so
            if (this->dtor_policy & DP_INLINE) {
                void *p =
                    this->alloc_raw(
                        INLINE_DTOR_OFFSET + (size * sizeof(T)));
                ASSERT(p);
                p_dtor = reinterpret_cast<DtorFn**>(p);
                *p_dtor = nullptr;
                ts =
                    reinterpret_cast<T*>(
                        reinterpret_cast<char*>(p) + INLINE_DTOR_OFFSET);
            } else {
                ts = reinterpret_cast<T*>(this->alloc_raw(size * sizeof(T)));
            }

            ASSERT(ts);
            ASSERT(p_size == nullptr);
        } else {
            constexpr auto OFFSET = MAX_ALIGN;

            // LAYOUT
            // p +                            0: size
            // p +                       OFFSET: dtor ptr (if present)
            // p + OFFSET + (INLINE_DTOR_SIZE?): ts

            void *p =
                this->alloc_raw(
                    OFFSET
                    + (this->dtor_policy & DP_INLINE ? INLINE_DTOR_OFFSET : 0)
                    + (size * sizeof(T)));
            ASSERT(p);
            p_size =
                reinterpret_cast<usize*>(
                    reinterpret_cast<char*>(p));
            p_dtor =
                (this->dtor_policy & DP_INLINE) ?
                    reinterpret_cast<DtorFn**>(
                        reinterpret_cast<char*>(p) + OFFSET)
                    : nullptr;
            ts =
                reinterpret_cast<T*>(
                    reinterpret_cast<char*>(p)
                        + OFFSET
                        + ((this->dtor_policy & DP_INLINE) ?
                            INLINE_DTOR_OFFSET
                            : 0));

            ASSERT(ts);

            DtorFn *dtor;
            if (this->dtor_policy & DP_INLINE) {
                // skip inline dtor as well
                dtor =
                    [](void *p) -> void* {
                        usize *p_size =
                            reinterpret_cast<usize*>(
                                reinterpret_cast<char*>(p)
                                    - OFFSET
                                    - INLINE_DTOR_OFFSET);
                        for (usize i = 0; i < *p_size; i++) {
                            reinterpret_cast<T*>(p)[i].~T();
                        }
                        return p_size;
                    };
            } else {
                // no need to skip inline dtor, allocator does not create them
                dtor =
                    [](void *p) -> void* {
                        usize *p_size =
                            reinterpret_cast<usize*>(
                                reinterpret_cast<char*>(p) - OFFSET);
                        for (usize i = 0; i < *p_size; i++) {
                            reinterpret_cast<T*>(p)[i].~T();
                        }
                        return p_size;
                    };
            }

            *p_size = size;

            if (p_dtor) {
                *p_dtor = dtor;
            }

            if (this->dtor_policy & DP_INLINE) {
                ASSERT(p_dtor);
                *p_dtor = dtor;
            }

            if (this->dtor_policy & DP_NOTIFY) {
                this->add_dtor(ts, dtor);
            }
        }

        if (flags & F_CALLOC) {
            std::memset(ts, 0, size * sizeof(T));
        }

        if constexpr (!std::is_trivially_default_constructible_v<T>) {
            new (ts) T[size];
        }

        return ts;
    }

    template <typename T>
    std::span<T> alloc_span(usize size, Flags flags = F_NONE) {
        if (size == 0) {
            return std::span<T> {};
        }

        return std::span { this->alloc_array<T>(size, flags), size };
    }

    template <typename T>
    std::span<T> copy_span(std::span<T> s) {
        if (s.size() == 0) {
            return std::span<T> {};
        }

        std::span<T> d = this->alloc_span<T>(s.size());

        if constexpr (std::is_trivially_copyable_v<T>) {
            std::memcpy(
                &d[0],
                &s[0],
                s.size_bytes());
        } else {
            std::copy(s.begin(), s.end(), d.begin());
        }

        return d;
    }

protected:
    u8 dtor_policy = DP_NOTHING;

    // gets pointer to inline dtor ptr for some allocated pointer p
    inline DtorFn **inline_dtor(void *p) {
        ASSERT(this->dtor_policy & DP_INLINE);
        return reinterpret_cast<DtorFn**>(
            reinterpret_cast<char*>(p) - INLINE_DTOR_OFFSET);
    }

    // allocate memory
    virtual void *alloc_raw(usize) { return nullptr; }

    // free a pointer allocated with alloc_raw
    virtual void free_raw(void *p) {}

    // called if dtor_policy & DP_NOTIFY on addition of an object with a dtor
    virtual void add_dtor(void*, DtorFn*) {}

    // called if dtor_policy & DP_NOTIFY on free-ing of an object with a dtor
    virtual void free_dtor(void*) {}
};
