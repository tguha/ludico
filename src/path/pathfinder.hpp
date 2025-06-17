#pragma once

#include "util/types.hpp"
#include "util/math.hpp"
#include "util/util.hpp"
#include "ext/inplace_function.hpp"
#include "path/path.hpp"

struct Pathfinder {
    using CanPathFn = std::function<bool(const ivec3 &, const ivec3 &)>;

    // cost of going FROM (arg 1) TO (arg 2)
    using CostFn = std::function<f32(const ivec3&, const ivec3 &)>;

    static f32 distance_cost(const ivec3 &a, const ivec3 &b);

    static std::optional<Path> find(
        const ivec3 &start,
        const ivec3 &end,
        usize limit,
        CanPathFn can_path_fn,
        CostFn cost_fn = distance_cost);
};
