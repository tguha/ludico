#pragma once

#include <utility>

// wrapper for T which is non-copyable
// when moved, other is initialized according to default constructor
template <typename T>
struct Moveable {
    Moveable() = default;
    Moveable(const Moveable<T> &other) = delete;
    Moveable(Moveable<T> &&other) {
        *this = std::move(other);
    }
    Moveable &operator=(const Moveable<T> &other) = delete;
    inline Moveable &operator=(Moveable<T> &&other) {
        this->t = other.t;
        other.t = T();
        return *this;
    }

    inline Moveable &operator=(const T &t) {
        this->t = t;
        return *this;
    }

    inline operator T() const {
        return this->t;
    }
private:
    T t = T();
};
