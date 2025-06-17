#include "ui/ui_inventory_combine.hpp"
#include "gfx/sprite_resource.hpp"

static const auto spritesheet =
    SpritesheetResource("ui_inventory", "ui/inventory.png", { 16, 16 });

static const auto sprite_battery_indicator =
    SpriteResource(
        [](const auto &_) {
            return spritesheet[{ 1, 2 }]
                .subpixels(
                    AABB2u(UIInventoryCombine::BatteryIndicator::SIZE - 1u));
        });

static const auto sprite_combine_icon =
    SpriteResource(
        [](const auto &_) {
            return spritesheet[{ 0, 2 }]
                .subpixels(AABB2u({ 8 - 1, 8 - 1 }));
        });

UIInventoryCombine::CombineIcon::CombineIcon()
    : Base(*sprite_combine_icon) {}

void UIInventoryCombine::BatteryIndicator::render(
    SpriteBatch &batch,
    RenderState render_state) {
    batch.push(
        *sprite_battery_indicator,
        this->pos_absolute());
}
