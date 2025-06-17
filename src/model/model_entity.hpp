#pragma once

#include "model/model.hpp"
#include "util/aabb.hpp"

struct Entity;

struct ModelEntity : public Model {
    ModelEntity() = default;
    ModelEntity(const ModelEntity&) = delete;
    ModelEntity(ModelEntity&&) = default;
    ModelEntity &operator=(const ModelEntity&) = delete;
    ModelEntity&operator=(ModelEntity&&) = default;
    virtual ~ModelEntity() = default;

    // get base aabb for model without any entity info
    virtual AABB aabb() const = 0;

    // get entity-specific AABB shape
    // NOTE: should NOT be translated to entity position, but should give the
    // correct (positioned) AABB when translated by entity position
    virtual AABB entity_aabb(const Entity &entity) const = 0;

    // OPTIONAL color provider
    virtual std::span<const vec3> colors() const {
        return std::span<const vec3>();
    }
};

// TODO: remove completely?
/* struct ModelVoxelEntity : public ModelEntity { */
/*     VoxelMesh mesh; */

/*     explicit ModelVoxelEntity( */
/*         VoxelMesh &&mesh) */
/*         : mesh(std::move(mesh)) {} */

/*     ModelVoxelEntity( */
/*         const MultiTexture &texture, */
/*         const TextureArea &area_tex, */
/*         const TextureArea &area_spec, */
/*         int depth) { */
/*         this->mesh = */
/*             VoxelMesh( */
/*                 VoxelModel::from_sprite( */
/*                     texture, */
/*                     area_tex, */
/*                     area_spec, */
/*                     depth)); */
/*     } */

/*     // TODO: remove? */
/*     // render model */
/*     // returns false if render was skipped at some stage */
/*     virtual bool render( */
/*         const Entity &entity, */
/*         RenderState render_state) const = 0; */

/*     AABB aabb() const override; */

/*     AABB entity_aabb(const Entity &entity) const override; */
/* }; */
