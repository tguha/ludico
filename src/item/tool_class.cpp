#include "item/tool_class.hpp"
#include "locale.hpp"

const ToolClass &ToolClass::get(Class c) {
    static const ToolClass classes[] = {
        [CLASS_COPPER] =
            ToolClass("copper", Locale::instance()["item:tool_copper_fmt"], 1.0f),
    };
    return classes[c];
}
