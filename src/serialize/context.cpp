#include "serialize/context.hpp"
#include "serialize/annotations.hpp"
#include "global.hpp"

SERIALIZE_ENABLE()
SERIALIZABLE_TYPE(SerializationContext)

Allocator &SerializationContext::allocator(arc::type_id id) {
    return global.allocator;
}
