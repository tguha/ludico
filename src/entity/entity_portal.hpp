/* #pragma once */

/* #include "entity/entity_type.hpp" */

/* struct ECPortal; */

/* struct EntityPortal : public EntityType<EntityPortal, ECPortal&&> { */
/*     using Base = EntityType<EntityPortal, ECPortal&&>; */
/*     ENTITY_TYPE_HEADER_MIXIN(EntityPortal, Base, "portal") */

/*     Entity create( */
/*         Entity e, ECPortal &&ec_portal) const override; */

/*     std::optional<Entity> spawn( */
/*         Level &level, ECPortal &&ec_portal) const override; */

/*     bool can_spawn( */
/*         const Level &level, const vec3 &pos, ECPortal &&ec_portal) const override; */

/*     std::optional<Entity> try_spawn( */
/*         Level &level, const vec3 &pos, ECPortal &&ec_portal) const override; */
/* }; */
