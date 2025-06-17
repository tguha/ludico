#include "item/item_metadata.hpp"
#include "serialize/serialize.hpp"

SERIALIZE_ENABLE()
DECL_SERIALIZER(ItemMetadata)
DECL_PRIMITIVE_SERIALIZER(alloc_ptr<ItemMetadata>)
