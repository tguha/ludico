#pragma once

#include "util/allocator.hpp"
#include "util/assert.hpp"

// TODO: no allocation pointer if T is not polymorphic?
// TODO: "free" is no-op if allocator does not need it
// TODO: doc
template <typename T>
struct alloc_ptr {
    template <typename U>
    friend struct alloc_ptr;

    alloc_ptr() = default;

    alloc_ptr(const alloc_ptr&) = delete;
    alloc_ptr &operator=(const alloc_ptr&) = delete;

    alloc_ptr(T *ptr, Allocator *allocator, void *alloc = nullptr)
        : _ptr(ptr),
          _allocator(allocator),
          _alloc(alloc ? alloc : ptr) {}

    alloc_ptr(alloc_ptr &&other) {
        *this = std::move(other);
    }

    alloc_ptr &operator=(alloc_ptr &&other) {
        if (*this) {
            this->~alloc_ptr();
        }

        this->_ptr = other._ptr;
        this->_allocator = other._allocator;
        this->_alloc = other._alloc;
        other._ptr = nullptr;
        other._allocator = nullptr;
        other._alloc = nullptr;
        return *this;
    }

    template <typename U>
        requires std::is_convertible_v<U, T>
    alloc_ptr(alloc_ptr<U> &&other) {
        *this = std::move(other);
    }

    template <typename U>
        requires std::is_convertible_v<U, T>
    alloc_ptr &operator=(alloc_ptr<U> &&other) {
        if (*this) {
            this->~alloc_ptr();
        }

        this->_ptr = (T*) (other._ptr);
        this->_allocator = other._allocator;
        this->_alloc = other._alloc;
        other._ptr = nullptr;
        other._allocator = nullptr;
        other._alloc = nullptr;
        return *this;
    }

    ~alloc_ptr() {
        if (*this) {
            this->_allocator->free(this->_alloc);
        }
    }

    inline operator bool() const {
        return this->_ptr != nullptr;
    }

    inline T &operator*() const {
        return *this->_ptr;
    }

    inline T *operator->() const {
        return this->_ptr;
    }

    template <typename U>
    inline bool operator==(const alloc_ptr<U> &rhs) const {
        return this->get() == rhs.get();
    }

    inline bool operator==(std::nullptr_t) const {
        return this->get() == nullptr;
    }

    template <typename U>
    inline auto operator<=>(const alloc_ptr<U> &rhs) const {
        return std::compare_three_way{}(this->get(), rhs.get());
    }

    inline auto operator<=>(std::nullptr_t) const {
        return std::compare_three_way{}(this->get(), (T*)(nullptr));
    }

    template <typename I>
    inline auto operator[](I &&i) const {
        ASSERT(*this);
        return (*this)[std::forward<I>(i)];
    }

    template <typename U>
    inline alloc_ptr<U> dyncast() && {
        U *u = dynamic_cast<U*>(this->get());
        ASSERT(
            u,
            "alloc_ptr dyncast from {} to {} failed",
            typeid(this->_ptr).name(),
            typeid(U).name());
        alloc_ptr<U> result(u, this->_allocator, this->_alloc);
        this->_ptr = nullptr;
        this->_allocator = nullptr;
        this->_alloc = nullptr;
        return result;
    }

    inline T *get() const {
        return this->_ptr;
    }

    inline Allocator *allocator() const {
        return this->_allocator;
    }

    inline void clear() {
        if (*this) {
            this->_allocator->free(this->_alloc);
        }

        this->_ptr = nullptr;
        this->_allocator = nullptr;
        this->_alloc = nullptr;
    }

private:
    T *_ptr = nullptr;
    Allocator *_allocator = nullptr;

    // TODO: no need for _alloc of T is not polymorphic
    void *_alloc = nullptr;
};

template <typename T, typename ...Args>
alloc_ptr<T> make_alloc_ptr(Allocator &allocator, Args&& ...args) {
    T *t = allocator.alloc<T>(std::forward<Args>(args)...);
    return alloc_ptr<T>(t, &allocator, t);
}
