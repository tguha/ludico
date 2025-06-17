#include "ui/ui_inventory_gear.hpp"
#include "gfx/sprite_resource.hpp"

static const auto spritesheet =
    SpritesheetResource("ui_inventory", "ui/inventory.png", { 16, 16 });

UIInventoryGear::LogicBoardSlot&
    UIInventoryGear::LogicBoardSlot::configure(bool enabled) {
    Base::configure(enabled);
    this->color = UIItemSlotBasic::SLOT_COLOR_GOLD;
    this->overlay = spritesheet[{ 3, 1 }].subpixels(AABB2u({ 8 - 1, 8 - 1 }));
    this->slot_ref =
        &this->gear()
            .inventory()
            .player()
            .inventory()
            ->logic_board();
    this->reposition();
    return *this;
}

UIInventoryGear::ToolsGrid&
    UIInventoryGear::ToolsGrid::configure(bool enabled) {
    Base::configure(SIZE, enabled);
    for (usize i = 0; i < this->elements.size(); i++) {
        this->elements[i]->overlay =
            spritesheet[uvec2(i, 1)].subpixels(AABB2u({ 8 - 1, 8 - 1 }));
    }
    return *this;
}
