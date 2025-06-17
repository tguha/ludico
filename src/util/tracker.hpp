#pragma once

#include "util/util.hpp"
#include "util/types.hpp"
#include "util/assert.hpp"

// tracks an object of type T as its this pointer moves. "Tracker<T>" should be
// a field on T.
// Tracker<T> gives out references of type Tracker<T>::Ref which act as pointers
// which track the movement of the object T which the tracker lives on.
template <typename T>
struct Tracker {
private:
    struct PtrToT {
        T *value;
    };

    std::ptrdiff_t offset_of_this;
    std::shared_ptr<PtrToT> ptr_to_t;

    T *ptr_from_offset() const {
        return reinterpret_cast<T*>((((u8*) this) - this->offset_of_this));
    }

public:
    struct Ref {
        Ref() = default;
        explicit Ref(std::weak_ptr<PtrToT> &&ptr)
            : ptr(std::move(ptr)) {
            ASSERT(!this->ptr.expired());
        }

        inline bool operator==(const Ref &rhs) const {
            return this->get() == rhs.get();
        }

        inline operator bool() const {
            return !this->ptr.expired();
        }

        inline T *operator->() const {
            return this->ptr.expired() ? nullptr : this->ptr.lock()->value;
        }

        inline T &operator*() const {
            return *(this->ptr.lock()->value);
        }

        inline T *get() const {
            return this->ptr.expired() ? nullptr : this->ptr.lock()->value;
        }

private:
        std::weak_ptr<PtrToT> ptr;
    };

    Tracker() = default;

    Tracker(T *ptr, Tracker<T> T::*member)
        : offset_of_this(types::offset_of(member)) {
        this->ptr_to_t = std::make_shared<PtrToT>(PtrToT { ptr });
    }

    Tracker(const Tracker<T> &) = delete;
    Tracker<T> &operator=(const Tracker<T> &) = delete;

    Tracker(Tracker<T> &&other) {
        *this = std::move(other);
    }

    Tracker<T> &operator=(Tracker<T> &&other) {
        this->offset_of_this = other.offset_of_this;
        this->ptr_to_t = std::move(other.ptr_to_t);
        if (this->ptr_to_t) {
            this->ptr_to_t->value = this->ptr_from_offset();
        }
        return *this;
    }

    Ref ref() const {
        return Ref { std::weak_ptr<PtrToT>(this->ptr_to_t) };
    }
};
