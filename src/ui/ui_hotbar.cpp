#include "ui/ui_hotbar.hpp"
#include "ui/ui_item_description.hpp"
#include "entity/entity_player.hpp"
#include "gfx/sprite_resource.hpp"
#include "input/input.hpp"
#include "platform/platform.hpp"
#include "global.hpp"

static const auto spritesheet =
    SpritesheetResource("ui_hotbar", "ui/hotbar.png", { 16, 16 });

static const auto sprite_base =
    SpriteResource(
        [](const auto &_) {
            const auto &sprites = spritesheet.get();
            constexpr auto OFFSET_BG = uvec2(0, 1);
            return
                sprites[
                    AABB2u(
                        OFFSET_BG,
                        OFFSET_BG + uvec2(sprites.size_units.x - 1, 0))]
                    .subpixels(
                        AABB2u(
                            uvec2(0, 0),
                            UIHotbar::SIZE - 1u));
        });

UIHotbar &UIHotbarSlot::hotbar() const {
    return dynamic_cast<UIHotbar&>(*this->parent);
}

UIHotbarSlot &UIHotbarSlot::configure(bool enabled) {
    Base::configure(enabled);
    this->alternate.base = spritesheet[{ 0, 0 }];
    this->alternate.highlight = spritesheet[{ 3, 0 }];
    return *this;
}

void UIHotbarSlot::render(SpriteBatch &batch, RenderState render_state) {
    Base::render(batch, render_state);

    if (this->hotbar().index == this->index) {
        batch.push(spritesheet[{ 2, 0 }], this->pos_absolute());
    }
}

ItemContainer::Slot &UIHotbar::selected() const {
    const auto &slot = this->elements[this->index];
    ASSERT(slot->slot_ref);
    return *slot->slot_ref;
}

UIHotbar &UIHotbar::configure(bool enabled) {
    Base::configure(sprite_base.get().size(), enabled);
    this->reposition();
    return *this;
}

void UIHotbar::reposition() {
    Base::reposition();
    this->align(UIObject::ALIGN_CENTER_H | UIObject::ALIGN_BOTTOM);
    this->pos += UIHUD::HOTBAR_OFFSET;
}

void UIHotbar::render(SpriteBatch &batch, RenderState render_state) {
    batch.push(
        sprite_base.get(),
        this->pos_absolute());
    Base::render(batch, render_state);
}

void UIHotbar::tick() {
    Base::tick();

    const auto &mouse = global.platform->get_input<Mouse>();
    const auto &keyboard = global.platform->get_input<Keyboard>();

    isize d_selected = 0;
    if (mouse.scroll_delta_tick > 0.1f) {
        d_selected = 1;
    } else if (mouse.scroll_delta_tick < -0.1f) {
        d_selected = -1;
    }

    const auto index_old = this->index;
    this->index =
        ((isize(this->index) + d_selected) + this->num_elements())
            % this->num_elements();

    const auto keys = types::make_array(1, 2, 3, 4, 5);
    for (usize i = 0; i < keys.size() && i < this->num_elements(); i++) {
        std::string s;
        s.append(1, static_cast<char>('0' + keys[i]));

        if (keyboard[s]->is_pressed_tick()) {
            this->index = i;
        }
    }

    if (index_old != this->index) {
        const auto &selected = this->selected();

        if (!selected.empty()) {
            this->hud().item_description->set(
                UIItemDescription::Entry(*selected, 40));
        }
    }

    this->hud().player().set_held_slot(this->selected());
}

void UIHotbar::reposition_element(UIHotbarSlot &slot, const uvec2 &index) {
    slot.pos = BASE_SLOT_OFFSET + (SLOT_OFFSET * index);
}

void UIHotbar::create_element(UIHotbarSlot &slot, const uvec2 &index) {
    slot.index = index.x;
    slot.slot_ref = this->hud().player().inventory()->hotbar()[index.x];
}
