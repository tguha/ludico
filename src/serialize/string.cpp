#include "serialize/string.hpp"
#include "serialize/serialize.hpp"

SERIALIZE_ENABLE()
// needed because of extra template parameter
// TODO: make DECL_PRIMITIVE_SERIALIZER_OPT more robust to handle this case?
ARCHIMEDES_REFLECT_TYPE(
    SerializerImpl<
        std::string,
        Serializer<std::string>>)
DECL_PRIMITIVE_SERIALIZER_OPT(std::string)
