#pragma once

#include "serialize/serializer.hpp"
#include "serialize/math.hpp"
#include "util/util.hpp"

template <typename T, usize N>
struct [[SERIALIZER]] Serializer<std::array<T, N>>
    : Serializer<T>,
      SerializerImpl<std::array<T, N>> {
    using Base = Serializer<T>;

    void serialize(
        const std::array<T, N> &arr,
        Archive &archive,
        SerializationContext &ctx) const {
        for (const auto &e : arr) {
            Base::serialize(e, archive, ctx);
        }
    }

    void deserialize(
        std::array<T, N> &arr,
        Archive &archive,
        SerializationContext &ctx) const {
        for (usize i = 0; i < N; i++) {
            Base::deserialize(arr[i], archive, ctx);
        }
    }
};
