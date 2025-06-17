#pragma once

#include "ui/ui_object.hpp"
#include "item/item_ref.hpp"

struct UIItemSlot : public UIObject {
    bool highlight = false;

    // TODO: better name, 'disabled' is unclear
    bool disabled = false;

    ItemContainer::SlotRef slot_ref = ItemContainer::SlotRef::none();

    // ref of current item in slot, ItemRef::none() if not present
    inline ItemRef ref() const {
        return this->slot_ref ?
            static_cast<ItemRef>(this->slot_ref.slot())
            : ItemRef::none();
    }

    UIItemSlot &configure(bool enabled = true);

    bool hover(const ivec2 &pos) override;

    bool click(const ivec2 &pos, bool is_release) override;

    void render_stack(
        SpriteBatch &batch, RenderState render_state) const;

    void render_stack_indicator(
        SpriteBatch &batch, RenderState render_state, const ivec2 &pos) const;

    virtual uvec2 get_size() const = 0;
};
