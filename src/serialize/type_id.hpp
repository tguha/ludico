#pragma once

#include <archimedes/type_id.hpp>

#include "serialize/annotations.hpp"
#include "serialize/serializer.hpp"
#include "serialize/math.hpp"

namespace arc = archimedes;

// type_id serializer
template <>
struct [[SERIALIZER]] Serializer<arc::type_id>
    : Serializer<arc::type_id::internal>,
      SerializerImpl<arc::type_id> {
    void serialize(
        const arc::type_id &id,
        Archive &archive,
        SerializationContext &ctx) const {
        Serializer<arc::type_id::internal>::serialize(id.value(), archive, ctx);
    }

    void deserialize(
        arc::type_id &id,
        Archive &archive,
        SerializationContext &ctx) const {
        arc::type_id::internal i;
        Serializer<arc::type_id::internal>::deserialize(i, archive, ctx);
        id = arc::type_id::from(i);
    }
};
