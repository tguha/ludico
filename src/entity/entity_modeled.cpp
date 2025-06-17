#include "entity/entity_modeled.hpp"
#include "model/model_entity.hpp"
#include "serialize/serialize.hpp"

SERIALIZE_ENABLE()
DECL_SERIALIZER(EntityModeled)

std::optional<AABB> EntityModeled::aabb() const {
    return this->model().entity_aabb(*this).translate(this->pos);
}
