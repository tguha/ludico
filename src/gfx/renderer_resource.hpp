#pragma once

#include "util/managed_resource.hpp"

namespace detail {
    void add_renderer_resource(IManagedResource*);
}

template <typename T>
struct RendererResource : public ManagedResource<T> {
    using ManagedResource<T>::ManagedResource;

    void attach() override {
        detail::add_renderer_resource(static_cast<IManagedResource*>(this));
    }
};


template <typename T>
struct SharedRendererResource : public SharedManagedResource<T> {
    using SharedManagedResource<T>::SharedManagedResource;

    void attach() override {
        detail::add_renderer_resource(static_cast<IManagedResource*>(this));
    }
};
