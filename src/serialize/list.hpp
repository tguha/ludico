#pragma once

#include "serialize/serializer.hpp"
#include "serialize/math.hpp"
#include "util/list.hpp"
#include "util/util.hpp"

template <typename L, typename T, typename Traits>
struct [[SERIALIZER]] Serializer<ListBase<L, T, Traits>>
    : Serializer<T>,
      SerializerImpl<ListBase<L, T, Traits>> {
    using Base = Serializer<T>;
    using ListType = ListBase<L, T, Traits>;

    void serialize(
        const ListType &list,
        Archive &archive,
        SerializationContext &ctx) const {
        Serializer<usize>().serialize(list.size(), archive, ctx);
        for (const auto &l : list) {
            Base::serialize(l, archive, ctx);
        }
    }

    void deserialize(
        ListType &list,
        Archive &archive,
        SerializationContext &ctx) const {
        usize sz;
        Serializer<usize>().deserialize(sz, archive, ctx);
        list.resize(sz);
        for (usize i = 0; i < sz; i++) {
            Base::deserialize(list[i], archive, ctx);
        }
    }
};
