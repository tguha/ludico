#pragma once

#include "ui/cursor_attachment.hpp"
#include "item/item_container.hpp"
#include "item/item_stack.hpp"
#include "entity/entity.hpp"

// TODO: allow to cancel
struct CursorAttachmentItem : public CursorAttachment {
    ItemStack stack;
    ItemContainer::Slot &origin;

    CursorAttachmentItem(
        ItemStack &&stack,
        ItemContainer::Slot &origin)
        : stack(std::move(stack)),
          origin(origin) {}

    void render(
        const ivec2 &cursor,
        SpriteBatch &batch,
        RenderState render_state) override;

    void tick(const ivec2 &cursor) override;

    void on_click(
        bool left,
        UIObject *obj) override;
};
