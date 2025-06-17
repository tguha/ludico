#pragma once

#include "serialize/annotations.hpp"
#include "serialize/serializer.hpp"
#include "serialize/math.hpp"

// optional serializer
template <typename T>
struct [[SERIALIZER]] Serializer<std::optional<T>>
    : Serializer<T>,
      SerializerImpl<std::optional<T>> {
    void serialize(
        const std::optional<T> &data,
        Archive &archive,
        SerializationContext &ctx) const {
        Serializer<u8>().serialize(data ? 0xFF : 0x00, archive, ctx);
        if (data) {
            Serializer<T>::serialize(*data, archive, ctx);
        }
    }

    void deserialize(
        std::optional<T> &data,
        Archive &archive,
        SerializationContext &ctx) const {
        u8 header;
        Serializer<u8>().deserialize(header, archive, ctx);
        if (header) {
            T t;
            Serializer<T>::deserialize(t, archive, ctx);
            data.emplace(std::move(t));
        }
    }
};
