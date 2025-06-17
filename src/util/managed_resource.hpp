#pragma once

#include "util/util.hpp"
#include "ext/inplace_function.hpp"
#include "util/types.hpp"

struct IManagedResource {
    virtual ~IManagedResource() = default;

    virtual void attach() {}

    virtual void destroy() {}
};

template <typename Base>
struct BaseManagedResource {
    template <typename T>
    inline auto &operator[](T &&t) const {
        return (*(const Base*)(this)).get()[std::forward<T>(t)];
    }

    template <typename T>
    inline auto &operator[](T &&t) {
        return (*(Base*)(this)).get()[std::forward<T>(t)];
    }

    inline auto &operator*() const {
        return (*(const Base*)(this)).get();
    }

    inline auto &operator*() {
        return (*(Base*)(this)).get();
    }

    inline auto operator->() const {
        return &(*(const Base*)(this)).get();
    }

    inline auto operator->() {
        return &(*(Base*)(this)).get();
    }
};

template<typename T>
struct ManagedResource
    : public IManagedResource,
      public BaseManagedResource<ManagedResource<T>> {
    using CreateFn = std::function<std::unique_ptr<T>(void)>;
    using InitFn = std::function<void(T&)>;

    CreateFn create_fn;
    std::optional<InitFn> init_fn;
    mutable std::unique_ptr<T> t;

    explicit ManagedResource(
        std::function<T(void)> &&create_fn,
        std::optional<InitFn> init_fn = std::nullopt)
            requires (std::is_move_constructible<T>::value)
        : create_fn(
            [create_fn = std::move(create_fn)] () {
                return std::make_unique<T>(std::move(create_fn()));
            }),
          init_fn(init_fn) {}

    explicit ManagedResource(
        CreateFn &&create_fn,
        std::optional<InitFn> init_fn = std::nullopt)
        : create_fn(std::move(create_fn)),
          init_fn(init_fn) {}

    void destroy() override {
        this->t.reset();
    }

    inline T &get() const {
        if (!this->t) {
            this->t = create_fn();
            const_cast<ManagedResource<T>*>(this)->attach();
            if (this->init_fn) {
                (*this->init_fn)(*this->t);
            }
        }

        return *this->t;
    }
};

template<typename T>
struct SharedManagedResource
    : public IManagedResource,
      public BaseManagedResource<ManagedResource<T>> {
    using CreateFn = std::function<std::shared_ptr<T>(void)>;

    CreateFn create_fn;
    mutable std::shared_ptr<T> t;

    explicit SharedManagedResource(CreateFn &&create_fn)
        : create_fn(std::move(create_fn)) {}

    void destroy() override {
        this->t.reset();
    }

    inline T &get() const {
        if (!this->t) {
            this->t = create_fn();
            const_cast<ManagedResource<T>*>(this)->attach();
            if (this->init_fn) {
                (*this->init_fn)(*this->t);
            }
        }

        return *this->t;
    }
};

template<typename T>
struct ValueManagedResource
    : public IManagedResource,
      public BaseManagedResource<ValueManagedResource<T>> {
    using CreateFn = std::function<T(void)>;
    using InitFn = std::function<void(T&)>;

    CreateFn create_fn;
    std::optional<InitFn> init_fn;
    mutable T t;
    mutable bool initialized = false;

    explicit ValueManagedResource(
        CreateFn &&create_fn,
        std::optional<InitFn> init_fn = std::nullopt)
        : create_fn(std::move(create_fn)),
          init_fn(init_fn) {}

    void destroy() override { }

    inline T &get() const {
        if (!this->initialized) {
            this->t = std::move(create_fn());
            const_cast<ValueManagedResource<T>*>(this)->attach();
            if (this->init_fn) {
                (*this->init_fn)(this->t);
            }
            this->initialized = true;
        }

        return this->t;
    }
};

template<typename T>
struct ReferenceManagedResource
    : public IManagedResource,
      public BaseManagedResource<ReferenceManagedResource<T>> {
    using CreateFn = std::function<T&(void)>;
    using InitFn = std::function<void(T&)>;

    CreateFn create_fn;
    std::optional<InitFn> init_fn;
    mutable T *t;
    mutable bool initialized = false;

    explicit ReferenceManagedResource(
        CreateFn &&create_fn,
        std::optional<InitFn> init_fn = std::nullopt)
        : create_fn(std::move(create_fn)),
          init_fn(init_fn) {}

    void destroy() override { }

    inline T &get() const {
        if (!this->initialized) {
            this->t = &create_fn();
            const_cast<ReferenceManagedResource<T>*>(this)->attach();
            if (this->init_fn) {
                (*this->init_fn)
                    (*const_cast<ReferenceManagedResource<T>*>(this)->t);
            }
            this->initialized = true;
        }

        return *this->t;
    }
};

namespace managed_resource {
// TODO: remove this?
template <typename T, typename ...Args>
ValueManagedResource<T> make_value_managed(Args&& ...args) {
    return ValueManagedResource<T>(
        [...brgs = std::forward<Args>(args)]() {
            return T(std::forward<Args>(brgs)...);
        });
}
}
