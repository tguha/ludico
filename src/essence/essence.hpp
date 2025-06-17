#pragma once

#include "util/util.hpp"
#include "util/types.hpp"

// essence types
enum class Essence {
    WATER = 0,
    FIRE,
    ICE,
    SOLAR,
    LUNAR,
    VOID,
    COUNT
};

// ItemStack equivalent for essences
struct EssenceStack {
    Essence kind;
    usize amount = 0;
};

// holder for a single essence
struct EssenceHolder {
    EssenceStack stack;
    usize max;
};
