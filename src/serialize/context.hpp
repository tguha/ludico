#pragma once

#include <archimedes/type_id.hpp>
#include <archimedes/any.hpp>
namespace arc = archimedes;

#include "util/util.hpp"

struct Allocator;

// base for all serialization contexts which is itself serializable
// the first thing which gets deserialized, last thing which gets serialized
struct SerializationContext {
    using ResolveFn = std::function<void(void)>;

    std::vector<ResolveFn> queued_resolutions;

    virtual ~SerializationContext() = default;

    // allocator for deserialized pointers (with type_id)
    virtual Allocator &allocator(arc::type_id id);

    // context is notified when record types are serialized
    virtual void notify_serialize(const arc::any &ptr, arc::type_id id) {}

    // context is notified when record types are deserialized
    virtual void notify_deserialize(const arc::any &ptr, arc::type_id id) {}

    // context is notified when record types are allocated but before they are
    // deserialized
    virtual void notify_alloc(const arc::any &ptr, arc::type_id id) {}

    // TODO: doc
    virtual void queue_resolve(ResolveFn &&callback) {
        this->queued_resolutions.emplace_back(std::move(callback));
    }

    // TODO: doc
    virtual void resolve() {
        for (const auto &f : this->queued_resolutions) {
            f();
        }
        this->queued_resolutions.clear();
    }
};
