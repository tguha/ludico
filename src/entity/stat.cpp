#include "entity/stat.hpp"
#include "serialize/serialize.hpp"

SERIALIZE_ENABLE()
DECL_SERIALIZER(Stat<float>)
// TODO: broken DECL_SERIALIZER(Stat<int>)
// TODO: broken DECL_SERIALIZER(Stat<uint>)
