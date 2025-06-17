#pragma once

#include "ui/ui_item_slot.hpp"
#include "ui/ui_root.hpp"

struct UIItemSlotBasic : public UIItemSlot {
    static constexpr auto
        Z_HIGHLIGHT = UIRoot::Z_MOUSE - 1;

    static constexpr auto
        SIZE = uvec2(12, 12);

    enum SlotColor {
        SLOT_COLOR_DEFAULT = 0,
        SLOT_COLOR_BLUE,
        SLOT_COLOR_RED,
        SLOT_COLOR_GOLD
    };

    // if present, overlay is rendered when there is nothing in the item slot
    // NOTE: expected to be item-sized (8x8)
    std::optional<TextureArea> overlay;

    // optional alternate textures
    struct {
        std::optional<TextureArea> base, disabled, highlight;
    } alternate;

    // color variant
    SlotColor color = SLOT_COLOR_DEFAULT;

    void render(SpriteBatch &batch, RenderState render_state) override;

    uvec2 get_size() const override {
        return SIZE;
    }
};
