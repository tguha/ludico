#include "serialize/item_ptr.hpp"
#include "item/item.hpp"

SERIALIZE_ENABLE()
SERIALIZABLE_TYPE(SerializerItemPtr)

void SerializerItemPtr::serialize(
    const Item* const& ptr,
    Archive &a,
    SerializationContext &ctx) {
    Serializer<ItemId>::serialize(ptr ? ptr->id : NO_ITEM, a, ctx);
}

void SerializerItemPtr::deserialize(
    const Item *&ptr,
    Archive &a,
    SerializationContext &ctx) {
    ItemId id;
    Serializer<ItemId>::deserialize(id, a, ctx);
    ptr = id == NO_ITEM ? nullptr : &Items::get()[id];
}
