#include "entity/entity_mob.hpp"
#include "level/level.hpp"
#include "model/model_entity.hpp"
#include "serialize/serialize.hpp"

SERIALIZE_ENABLE()
DECL_SERIALIZER(EntityMob)

bool EntityMob::can_reach(const vec3 &target) const {
    return math::length(this->pos - target) <= this->reach();
}

bool EntityMob::can_reach(const ivec3 &target) const {
    return this->can_reach(Level::to_tile_center(target));
}

void EntityMob::tick() {
    Base::tick();
    this->facing.tick();
}

LightArray EntityMob::lights() const {
    // TODO: include light from held item
    return Base::lights();
}
