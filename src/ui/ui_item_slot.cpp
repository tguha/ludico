#include "ui/ui_item_slot.hpp"
#include "ui/cursor_attachment_item.hpp"
#include "ui/ui_hud.hpp"
#include "ui/ui_item_description.hpp"
#include "ui/ui_root.hpp"
#include "gfx/sprite_resource.hpp"
#include "item/item_icon.hpp"
#include "util/optional.hpp"
#include "state/state_game.hpp"
#include "global.hpp"

static const auto spritesheet =
    SpritesheetResource("ui_item_slot", "ui/item_slot.png", { 16, 16 });

UIItemSlot &UIItemSlot::configure(bool enabled) {
    UIObject::configure(this->get_size(), enabled);
    return *this;
}

bool UIItemSlot::hover(const ivec2 &pos) {
    if (this->disabled) {
        return false;
    } else if (!this->ref()) {
        return true;
    }

    const auto &stack = *this->ref();
    auto &item_description = *global.game->hud->item_description;
    if (!item_description.is_set_this_tick()) {
        item_description.set(stack);
    }
    return true;
}

bool UIItemSlot::click(const ivec2 &pos, bool is_release) {
    if (!is_release) {
        return false;
    } if (!this->slot_ref) {
        return false;
    } else if (this->disabled) {
        return false;
    }

    // true if there is a cursor attachment currently, regardless of its type
    bool existing_attachment = this->root->cursor_attachment != nullptr;

    // current cursor item attachment, nullptr if not present
    CursorAttachmentItem *ca_item =
        dynamic_cast<CursorAttachmentItem*>(
            this->root->cursor_attachment.get());

    // will become the new stack the cursor is holding if present
    std::optional<ItemStack> new_cursor_stack;

    // replace with new stack if holding
    if (ca_item) {
        if (this->ref()
                && this->ref()->item().stack_size() > 1
                && this->ref()->item().id == ca_item->stack.item().id
                && this->ref()->item().stackable(
                    *this->ref(),
                    ca_item->stack)) {
            // try to stack with the existing item

            // add as many as possible, remainder becomes new cursor stack
            auto [_, remainder] =
                this->slot_ref->parent().try_add(
                    std::move(ca_item->stack),
                    this->slot_ref->index());

            if (remainder) {
                new_cursor_stack = std::move(*remainder);
            }

            this->root->cursor_attachment = nullptr;
            ca_item = nullptr;
            existing_attachment = false;
        } else if (
            this->slot_ref->parent().allowed(
                this->slot_ref->index(), ca_item->stack.item())) {
            // only if placement is allowed

            // save old stack, this will become the new cursor attachment
            if (this->ref()) {
                new_cursor_stack =
                    this->slot_ref->parent().remove(
                        *this->ref());
            }

            // replace with new stack
            *this->slot_ref = std::move(ca_item->stack);

            // invalidate cursor attachment
            this->root->cursor_attachment = nullptr;
            ca_item = nullptr;
            existing_attachment = false;
        }
    } else if (this->ref() && !existing_attachment) {
        // pick up stack, there is no cursor attachment
        new_cursor_stack =
            this->slot_ref->parent().remove(
                *this->ref());
    }

    // add cursor attachment if we have a stack to add AND there is no current
    // attachment
    if (new_cursor_stack && !existing_attachment) {
        this->root->cursor_attachment =
            std::make_unique<CursorAttachmentItem>(
                std::move(*new_cursor_stack),
                this->slot_ref);
    }

    return true;
}

void UIItemSlot::render_stack(
    SpriteBatch &batch,
    RenderState render_state) const {
    if (!this->ref()) {
        return;
    }

    const auto &stack = *this->ref();
    const auto &icon = stack.item().icon();
    if (const auto area = icon.texture_area(&stack)) {
        icon.render_icon(
            &stack,
            batch,
            this->pos_absolute()
                + (ivec2(this->size - area->pixels().size()) / 2),
            vec4(0.0f),
            this->z + 1);
    }
}

void UIItemSlot::render_stack_indicator(
    SpriteBatch &batch, RenderState render_state, const ivec2 &pos) const {
    if (!this->ref()) {
        return;
    }

    const auto &stack = *this->ref();

    if (stack.item().stack_size() <= 1) {
        return;
    }

    constexpr auto BAR_SIZE = uvec2(12, 3);
    constexpr auto BAR_WIDTH = 10;

    const auto &sprites = *spritesheet;
    const auto
        sprite_empty = sprites[{ 0, 2 }],

        sprite_full = sprites[{ 1, 2 }];

    const auto filled = math::max<usize>(
        usize(
            math::floor(
                (f32(stack.size) / stack.item().stack_size()) * BAR_WIDTH)),
        1);

    // render full bar from 0 to 1 + filled
    batch.push(
        sprite_full.subpixels(
            AABB2u(uvec2(0, 0), uvec2(filled, BAR_SIZE.y - 1))),
        pos,
        vec4(0.0f),
        1.0f,
        this->z + 1);

    // render empty bar for rest
    batch.push(
        sprite_empty.subpixels(
            AABB2u(uvec2(filled + 1, 0), BAR_SIZE - 1u)),
        pos + ivec2(filled + 1, 0),
        vec4(0.0f),
        1.0f,
        this->z + 1);
}
