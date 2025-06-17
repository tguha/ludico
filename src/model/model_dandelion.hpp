#pragma once

#include "model/model_flower_multi.hpp"

struct ModelDandelion : public ModelFlowerMulti {
    using Base = ModelFlowerMulti;

    ModelDandelion();

    std::span<const vec3> colors() const override;
};
