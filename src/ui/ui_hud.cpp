#include "ui/ui_hud.hpp"
#include "input/controls.hpp"
#include "ui/ui_inventory.hpp"
#include "ui/ui_hotbar.hpp"
#include "ui/ui_item_description.hpp"
#include "ui/ui_pointer_label.hpp"
#include "ui/ui_root.hpp"
#include "ui/ui_stats.hpp"
#include "ui/ui_event_player_change.hpp"
#include "entity/entity_player.hpp"
#include "gfx/renderer.hpp"
#include "util/time.hpp"
#include "global.hpp"

void UIHUD::set_player(EntityPlayer &player) {
    this->_player = player;
    this->on_event(UIEventPlayerChange(this->player()));
}

EntityPlayer &UIHUD::player() const {
    return *this->_player.as<EntityPlayer>();
}

UIHUD &UIHUD::configure(bool enabled) {
    Base::configure( this->root->size, enabled);
    this->reposition();

    this->item_description =
        &this->add(std::make_unique<UIItemDescription>())
            .configure(true);

    this->pointer_label =
        &this->add(std::make_unique<UIPointerLabel>())
            .configure(true);

    this->hotbar =
        &this->add(std::make_unique<UIHotbar>())
            .configure(true);

    this->stats =
        &this->add(std::make_unique<UIStats>())
            .configure(true);

    this->inventory =
        &this->add(std::make_unique<UIInventory>())
            .configure(false);

    return *this;
}

void UIHUD::tick() {
    Base::tick();

    if (global.controls->inventory.is_pressed_tick()) {
        if (this->inventory->is_enabled()) {
            this->inventory->set_enabled(false);
        } else {
            this->inventory->open();
        }
    }
}

void UIHUD::reposition() {
    this->size = this->root->size;
    Base::reposition();
}
