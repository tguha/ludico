#pragma once

#include "entity/entity.hpp"

struct ModelEntity;

struct EntityModeled : virtual public Entity {
    using Base = Entity;

    EntityModeled() = default;

    // model for this entity, nullptr if not present
    virtual const ModelEntity &model() const = 0;

    std::optional<AABB> aabb() const override;
};
