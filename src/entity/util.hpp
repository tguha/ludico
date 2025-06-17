#pragma once

#include "util/types.hpp"
#include "util/math.hpp"
#include "ext/inplace_function.hpp"

struct Entity;
struct Level;

using EntityId = u16;

// represents no entity id
constexpr auto NO_ENTITY = EntityId(0);

using EntityFilterFn = InplaceFunction<bool(const Entity&), 16>;

// spawns an entity in the specified level at position, returns true on success
using EntitySpawnFn = std::function<bool(Level&, const vec3&)>;
