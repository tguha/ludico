#include "ui/ui_button_arrow.hpp"
#include "gfx/sprite_resource.hpp"

static const auto spritesheet =
    SpritesheetResource("ui_button_arrow", "ui/button_arrow.png", { 16, 16 });

UIButtonArrow::UIButtonArrow(Type type)
    : Base(),
      type(type) {
    this->size = UIButtonArrow::size_of(type);

    const auto bounds = AABB2u({ 0, 0 }, this->size - 1u);
    this->sprites = {
        .base = spritesheet[uvec2(0, type)].subpixels(bounds),
        .hovered = spritesheet[uvec2(1, type)].subpixels(bounds),
        .down = spritesheet[uvec2(2, type)].subpixels(bounds),
        .disabled = spritesheet[uvec2(3, type)].subpixels(bounds),
    };
}
