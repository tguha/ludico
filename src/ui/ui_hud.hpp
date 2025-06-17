#pragma once

#include "ui/ui_container.hpp"
#include "ui/ui_event_player_change.hpp"
#include "entity/entity_ref.hpp"

struct UIItemDescription;
struct UIPointerLabel;
struct UIHotbar;
struct UIStats;
struct UIInventory;
struct UITools;
struct UICrafting;
struct EntityPlayer;

struct UIHUD
    : public UIContainer,
      public UIEventHandler<UIEventPlayerChange> {
    using Base = UIContainer;

    // must match ui_item_slot size
    static constexpr auto ENTRY_SIZE = uvec2(14);

    static constexpr auto HOTBAR_OFFSET = ivec2(0, 2);
    static constexpr auto STATS_OFFSET = ivec2(4, -4);

    UIItemDescription *item_description = nullptr;
    UIPointerLabel *pointer_label = nullptr;
    UIHotbar *hotbar = nullptr;
    UIStats *stats = nullptr;
    UIInventory *inventory = nullptr;
    UICrafting *crafting = nullptr;

    void set_player(EntityPlayer &player);

    EntityPlayer &player() const;

    bool is_ghost() const override { return true; }

    UIHUD &configure(bool enabled = true);

    void tick() override;

    void reposition() override;

private:
    EntityRef _player = nullptr;
};
