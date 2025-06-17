// TODO: archimedes crashes on entirely empty files
#include <archimedes.hpp>
ARCHIMEDES_ARG("disable")
#include "entity/entity_portal.hpp"
/* #include "entity/ec_name.hpp" */
/* #include "entity/ec_portal.hpp" */
/* #include "entity/ec_position.hpp" */
/* #include "entity/ec_render.hpp" */
/* #include "entity/ec_light.hpp" */
/* #include "entity/entity_render.hpp" */
/* #include "gfx/renderer_resource.hpp" */
/* #include "gfx/sprite_resource.hpp" */
/* #include "model/model_entity.hpp" */
/* #include "util/color.hpp" */
/* #include "util/time.hpp" */

/* ENTITY_TYPE_DECL(EntityPortal) */

/* static const auto PORTAL_AABB = AABB::unit(); */

/* static const auto spritesheet = */
/*     SpritesheetResource("entity_portal", "entity/portal.png", { 16, 16 }); */

/* static VoxelModel create_model(const uvec2 &offset) { */
/*     auto reshape = */
/*         [](VoxelShape shape) { */
/*             const auto copy = shape.deep_copy(); */

/*             for (usize x = 0; x < shape.size.x; x++) { */
/*                 for (usize y = 0; y < shape.size.y; y++) { */
/*                     for (usize z = 0; z < shape.size.z; z++) { */
/*                         const auto p = ivec3(x, y, z); */

/*                         if (y == 0) { */
/*                             continue; */
/*                         } else if (y > 2) { */
/*                             shape[p] = false; */
/*                             continue; */
/*                         } */

/*                         usize xz_neighbors = 0; */
/*                         for (const auto &d : Direction::CARDINAL) { */
/*                             const auto n = */
/*                                 ivec3(x, 0, z) + Direction::to_ivec3(d); */
/*                             if (copy.contains(n) && copy[n]) { */
/*                                 xz_neighbors++; */
/*                             } */
/*                         } */

/*                         shape[p] = shape[p] && xz_neighbors != 4; */
/*                     } */
/*                 } */
/*             } */
/*         }; */

/*     const auto &sprites = spritesheet.get(); */
/*     return */
/*         VoxelModel::from_sprites( */
/*             sprites, */
/*             { */
/*                 std::nullopt, */
/*                 std::nullopt, */
/*                 std::nullopt, */
/*                 std::nullopt, */
/*                 sprites[offset], */
/*                 sprites[offset], */
/*             }, */
/*             { */
/*                 Direction::UP, */
/*                 Direction::UP, */
/*                 Direction::UP, */
/*                 Direction::UP, */
/*                 Direction::UP, */
/*                 Direction::UP, */
/*             }, */
/*             VoxelModel::FLAG_NONE, */
/*             reshape); */
/* } */

/* static std::array<MultiMeshEntry::Id, ECPortal::VARIANT_COUNT> mesh_entry_ids; */

/* static auto model_manager = */
/*     RendererResource<MultiMesh>( */
/*         []() { return MultiMesh(VertexTextureNormal::layout); }, */
/*         [](MultiMesh &multi_mesh) { */
/*             for (usize i = 0; i < ECPortal::VARIANT_COUNT; i++) { */
/*                 mesh_entry_ids[i] = multi_mesh.add(create_model({ i, 0 })); */
/*             } */
/*         }); */

/* struct ECPortalRender : public ECRender { */
/*     void render(RenderEvent event) const override { */
/*         if (event.render_state.flags & RENDER_FLAG_PASS_TRANSPARENT) { */
/*             return; */
/*         } else if (entity_render::should_skip( */
/*                 event.entity, event.render_state)) { */
/*             return; */
/*         } */

/*         event.render_state = event.render_state.or_defaults(); */

/*         const auto entity = this->entity(); */
/*         const auto &program = Renderer::get().programs["model"]; */
/*         entity_render::prepare(entity, program, event.render_state); */

/*         auto model = mat4(1.0); */
/*         model = math::translate(model, entity.get<ECPosition>().pos); */
/*         model = math::scale(model, vec3(1.0f / SCALE)); */

/*         model_manager.get()[mesh_entry_ids[entity.get<ECPortal>().variant]] */
/*             .render( */
/*                 program, */
/*                 model, */
/*                 event.render_state); */
/*     } */
/* }; */

/* struct ECPortalLight : public ECLight { */
/*     LightArray lights() const override { */
/*         const auto entity = this->entity(); */
/*         const auto &ec_portal = entity.get<ECPortal>(); */
/*         const auto &ec_position = entity.get<ECPosition>(); */
/*         const auto */
/*             t = TICKS_PER_SECOND * 4.0f, */
/*             x = */
/*                 math::abs( */
/*                     math::sin( */
/*                         (math::TAU * ((global.time->ticks % usize(t)) / t)) */
/*                             + this->entity().id)), */
/*             s = 0.05f, */
/*             a = (1.0f - s) + (s * x); */
/*         return Light(ec_position.center, a * ec_portal.color, 10); */
/*     } */
/* }; */

/* Entity EntityPortal::create( */
/*     Entity e, ECPortal &&ec_portal) const { */
/*     e.add(ECName(Locale::instance()["entity:portal"])); */
/*     e.add(ECPosition()); */
/*     e.add(std::move(ec_portal)); */
/*     e.add( */
/*         ECCollision( */
/*             true, */
/*             PORTAL_AABB, */
/*             [](Entity e) { */
/*                 return true; */
/*             }, */
/*             [](Entity e) { */
/*                 return false; */
/*             })); */
/*     e.add(ECPortalRender()); */
/*     e.add(ECPortalLight()); */
/*     return e; */
/* } */

/* std::optional<Entity> EntityPortal::spawn( */
/*     Level &level, ECPortal &&ec_portal) const { */
/*     return this->create(TRY_OPT(level.create_entity()), std::move(ec_portal)); */
/* } */

/* bool EntityPortal::can_spawn( */
/*     const Level &level, const vec3 &pos, ECPortal &&ec_portal) const { */
/*     return this->can_spawn_with_box( */
/*         level, pos, PORTAL_AABB.translate(pos), std::move(ec_portal)); */
/* } */

/* std::optional<Entity> EntityPortal::try_spawn( */
/*     Level &level, const vec3 &pos, ECPortal &&ec_portal) const { */
/*     const auto e = */
/*         this->try_spawn_with_box( */
/*             level, pos, PORTAL_AABB.translate(pos), std::move(ec_portal)); */
/*     if (e) { */
/*         e->get<ECPosition>() = pos; */
/*     } */
/*     return e; */
/* } */
