#pragma once

#include "model/model_flower_multi.hpp"

struct ModelDaisy : public ModelFlowerMulti {
    using Base = ModelFlowerMulti;

    ModelDaisy();

    std::span<const vec3> colors() const override;
};
