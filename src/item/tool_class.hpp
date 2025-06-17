#pragma once

#include "util/util.hpp"
#include "util/types.hpp"

struct ToolClass {
    enum Class {
        CLASS_COPPER = 0,
        CLASS_COUNT
    };

    static inline const auto CLASSES =
        types::make_array(CLASS_COPPER);

    std::string name;
    std::string prefix_fmt;
    f32 effectiveness;

    ToolClass(
        const std::string &name,
        const std::string &prefix_fmt,
        f32 effectiveness)
        : name(name),
          prefix_fmt(prefix_fmt),
          effectiveness(effectiveness) {}

    static const ToolClass &get(Class c);
};
