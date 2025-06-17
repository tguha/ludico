#include "entity/entity_colored.hpp"
#include "serialize/serialize.hpp"

SERIALIZE_ENABLE()
DECL_SERIALIZER(EntityColored)
DECL_PRIMITIVE_SERIALIZER(decltype(EntityColored::_colors))
