#pragma once

#include "serialize/annotations.hpp"
#include "serialize/serializer.hpp"
#include "serialize/math.hpp"

// std::vector type serializer
template <typename T>
    requires types::is_vector<T>::value
struct [[SERIALIZER]] Serializer<T>
    : Serializer<typename T::value_type>,
      SerializerImpl<T> {
    using V = typename T::value_type;
    using Base = Serializer<V>;

    void serialize(
        const T &vs,
        Archive &archive,
        SerializationContext &ctx) const {
        SERIALIZE_LOG("Serializer<{}>::serialize (length {}) @ {}",
            arc::type_name<T>(),
            vs.size(),
            fmt::ptr(&vs));
        Serializer<usize>().serialize(vs.size(), archive, ctx);
        for (const auto &v : vs) {
            Serializer<V>::serialize(v, archive, ctx);
        }
    }

    void deserialize(
        T &vs,
        Archive &archive,
        SerializationContext &ctx) const {
        usize sz;
        Serializer<usize>().deserialize(sz, archive, ctx);
        vs.resize(sz);

        SERIALIZE_LOG("Serializer<{}>::deserialize (length {}) @ {}",
            arc::type_name<T>(),
            vs.size(),
            fmt::ptr(&vs));

        for (usize i = 0; i < sz; i++) {
            Serializer<V>::deserialize(vs[i], archive, ctx);
        }
    }
};
