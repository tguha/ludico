#pragma once

#include "serialize/serializer.hpp"
#include "serialize/math.hpp"
#include "item/util.hpp"

struct SerializerItemPtr
    : Serializer<ItemId> {
    using SerializedType = const Item*;

    void serialize(
        const Item* const&,
        Archive&,
        SerializationContext&);
    void deserialize(
        const Item *&,
        Archive&,
        SerializationContext&);
};
