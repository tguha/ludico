#pragma once

#include "serialize/annotations.hpp"
#include "serialize/serializer.hpp"
#include "serialize/math.hpp"

// enum type serializer
template <typename T>
    requires std::is_enum_v<T>
struct [[SERIALIZER]] Serializer<T>
    : Serializer<std::underlying_type_t<T>>,
      SerializerImpl<T> {
    using U = std::underlying_type_t<T>;

    void serialize(
        const T &data,
        Archive &archive,
        SerializationContext &ctx) const {
        SERIALIZE_LOG("serializing enum type {} @ {}",
            arc::type_name<T>(),
            fmt::ptr(&data));
        Serializer<U>::serialize(static_cast<U>(data), archive, ctx);
    }

    void deserialize(
        T &data,
        Archive &archive,
        SerializationContext &ctx) const {
        SERIALIZE_LOG("deserializing enum type {} @ {}",
            arc::type_name<T>(),
            fmt::ptr(&data));
        U u;
        Serializer<U>::deserialize(u, archive, ctx);
        data = static_cast<T>(u);
    }
};
