#include "ui/cursor_attachment_item.hpp"
#include "global.hpp"
#include "state/state_game.hpp"
#include "ui/ui_item_description.hpp"
#include "ui/ui_object.hpp"
#include "ui/ui_hud.hpp"
#include "ui/ui_root.hpp"
#include "entity/entity_item.hpp"
#include "item/item_icon.hpp"
#include "item/item_container.hpp"

void CursorAttachmentItem::render(
    const ivec2 &cursor,
    SpriteBatch &batch,
    RenderState render_state) {
    const auto &icon = this->stack.item().icon();
    if (const auto area = icon.texture_area(&this->stack)) {
        icon.render_icon(
            &this->stack,
            batch,
            cursor - (ivec2(area->pixels().size()) / 2),
            vec4(0.0f),
            UIRoot::Z_MOUSE);
    }
}

void CursorAttachmentItem::tick(const ivec2 &cursor) {
    auto &item_desc = *global.game->hud->item_description;
    item_desc.set(this->stack);
}

void CursorAttachmentItem::on_click(
    bool left,
    UIObject *obj) {
    // drop an object clicking outside of the UI
    if ((obj == nullptr || dynamic_cast<UIHUD*>(obj))) {
        if (Entity *entity = this->origin.parent().owner) {
            if (auto *inventory = entity->inventory()) {
                EntityItem::drop_from_entity(
                    *entity,
                    std::move(this->stack));
                this->remove();
            }
        }
    }
}
