#pragma once

#include "ui/ui_hud.hpp"
#include "ui/ui_container.hpp"
#include "ui/ui_item_slot_basic.hpp"
#include "ui/ui_item_grid.hpp"
#include "entity/entity.hpp"

struct UIHotbar;
struct ItemStack;

struct UIHotbarSlot : public UIItemSlotBasic {
    using Base = UIItemSlotBasic;

    // index in hotbar
    usize index = 0;

    UIHotbar &hotbar() const;

    UIHotbarSlot &configure(bool enabled = true);

    void render(SpriteBatch &batch, RenderState render_state) override;
};

struct UIHotbar
    : public UIItemGrid<UIHotbarSlot>,
      public UIEventHandler<UIEventPlayerChange> {
    using Base = UIItemGrid<UIHotbarSlot>;

    static constexpr auto NUM_ENTRIES = 5;
    static constexpr auto SIZE = uvec2(84, 14);
    static constexpr auto
        BASE_SLOT_OFFSET = uvec2(6, 1),
        SLOT_OFFSET = uvec2(15, 0);

    usize index = 0;

    UIHotbar()
        : Base({ NUM_ENTRIES, 1 }) {}

    UIHUD &hud() const {
        return dynamic_cast<UIHUD&>(*this->parent);
    }

    void on_event(const UIEventPlayerChange &event) override {
        UIEventHandler<UIEventPlayerChange>::on_event(event);
        this->make_grid();
    }

    ItemContainer::Slot &selected() const;

    UIHotbar &configure(bool enabled = true);

    void reposition() override;

    void render(SpriteBatch &batch, RenderState render_state) override;

    void tick() override;

    void reposition_element(UIHotbarSlot &slot, const uvec2 &index) override;

    void create_element(UIHotbarSlot &slot, const uvec2 &index) override;
};
