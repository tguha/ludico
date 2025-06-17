#include "item/item_hammer.hpp"
#include "item/item_icon.hpp"
#include "gfx/sprite_resource.hpp"

DECL_ITEM_REGISTER(ItemHammerCopper)

static const auto spritesheet =
    SpritesheetResource("item_hammer", "item/hammer.png", { 8, 8 });

const ItemIcon &ItemHammerCopper::icon() const {
    static auto icon = ItemIconBasic(spritesheet[{ 0, 0 }]);
    return icon;
}
