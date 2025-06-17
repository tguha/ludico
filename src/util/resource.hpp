#pragma once

// MODIFIED BY jdh
// adds default constructor
// makes deleter an std::optional
// lots of other things :-)
//
// Resource
// Copyright (c) 2015 okdshin
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
// This implementation is based on C++ standards committee paper N4189.

#include <type_traits>
#include <utility>
#include <optional>

// workaround for GCC
#if defined(__GNUC__)
#ifdef UNIQUE_RESOURCE_ALLOW_DELETER_CALL_THROWING_EXCEPTION
#define UNIQUE_RESOURCE_NOEXCEPT
#else
#define UNIQUE_RESOURCE_NOEXCEPT noexcept
#endif
#define UNIQUE_RESOURCE_NOEXCEPT_NOEXCEPT_THIS_RESET UNIQUE_RESOURCE_NOEXCEPT
#define UNIQUE_RESOURCE_NOEXCEPT_NOEXCEPT_THIS_DELETER_CALL                    \
  UNIQUE_RESOURCE_NOEXCEPT
#else
#define UNIQUE_RESOURCE_NOEXCEPT_NOEXCEPT_THIS_RESET                           \
  noexcept(noexcept(this->reset()))
#define UNIQUE_RESOURCE_NOEXCEPT_NOEXCEPT_THIS_DELETER_CALL                    \
  noexcept(noexcept(this->get_deleter()(resource)))
#endif

template<typename R, typename D>
class Resource {
    R resource;
    std::optional<D> deleter;
    bool execute_on_destruction; // exposition only
public:
    Resource &operator=(Resource const &) = delete;
    Resource(Resource const &) = delete;

    template<typename S = R>
    requires std::is_default_constructible<S>::value
    Resource() noexcept
        : resource(R()),
          deleter(std::nullopt),
          execute_on_destruction(false) {}

    // construction
    Resource(R &&resource_, D &&deleter_, bool should_run = true) noexcept
        : resource(std::move(resource_)),
          deleter(std::move(deleter_)),
          execute_on_destruction(should_run) {}

    // move
    Resource(Resource &&other) noexcept
        : resource(std::move(other.resource)),
          deleter(std::move(other.deleter)),
          execute_on_destruction{ other.execute_on_destruction } {
        other.release();
    }

    Resource &operator=(Resource &&other)
    UNIQUE_RESOURCE_NOEXCEPT_NOEXCEPT_THIS_RESET {
        this->reset();
        this->deleter = std::move(other.deleter);
        this->resource = std::move(other.resource);
        this->execute_on_destruction = other.execute_on_destruction;
        other.release();
        return *this;
    }

    ~Resource() UNIQUE_RESOURCE_NOEXCEPT_NOEXCEPT_THIS_RESET {
        this->reset();
    }

    void reset() UNIQUE_RESOURCE_NOEXCEPT_NOEXCEPT_THIS_DELETER_CALL {
        if (execute_on_destruction) {
            this->execute_on_destruction = false;
            this->get_deleter()(resource);
        }
    }

    void reset(R &&new_resource) UNIQUE_RESOURCE_NOEXCEPT_NOEXCEPT_THIS_RESET {
        this->reset();
        this->resource = std::move(new_resource);
        this->execute_on_destruction = true;
    }

    R const &release() noexcept {
        this->execute_on_destruction = false;
        return this->get();
    }

    // resource access
    R const &get() const noexcept { return this->resource; }

    operator R const &() const noexcept { return this->resource; }

    R operator->() const noexcept { return this->resource; }

    typename std::add_lvalue_reference<
        typename std::remove_pointer<R>::type>::type
    operator*() const {
        return *this->resource;
    }

    const D &get_deleter() const noexcept { return this->deleter.value(); }
};

// reference-deleted resource
template <typename T>
using RDResource = Resource<T, std::function<void(T&)>>;