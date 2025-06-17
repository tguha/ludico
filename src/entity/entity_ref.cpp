#include "entity/entity_ref.hpp"
#include "entity/entity.hpp"
#include "level/level.hpp"
#include "serialize/context_level.hpp"
#include "serialize/serialize.hpp"

SERIALIZE_ENABLE()
DECL_SERIALIZER_OPT(BaseEntityRef)

Entity *detail::entity_from_id(const Level *level, EntityId id) {
    return level ? level->entity_from_id(id) : nullptr;
}

Level *detail::get_level(const Entity *e) {
    return e ? e->level : nullptr;
}

EntityId detail::get_id(const Entity *e) {
    return e ? e->id : NO_ENTITY;
}

std::string detail::name(const Entity *e) {
    return e ? e->name() : "NO_NAME";
}

void BaseEntityRef::on_after_deserialize(SerializationContextLevel &ctx) {
    this->level = ctx.level;
}
