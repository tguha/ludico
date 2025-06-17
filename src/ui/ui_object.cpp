#include "ui/ui_object.hpp"
#include "input/input.hpp"
#include "platform/platform.hpp"
#include "ui/ui_root.hpp"
#include "ui/ui_container.hpp"
#include "global.hpp"

UIObject::UIObject() {
    if (global.ui) {
        this->root = global.ui;
        this->id = global.ui->next_id();
    }
}

UIObject &UIObject::align(
    usize alignment,
    UIObject *relative_to) {

    if (!relative_to) {
        relative_to = this->parent;
    }

    ASSERT(relative_to);

    if (alignment & ALIGN_LEFT) {
        this->pos.x = 0;
    }

    if (alignment & ALIGN_RIGHT) {
        this->pos.x = relative_to->size.x - this->size.x;
    }

    if (alignment & ALIGN_BOTTOM) {
        this->pos.y = 0;
    }

    if (alignment & ALIGN_TOP) {
        this->pos.y = relative_to->size.y - this->size.y;
    }

    if (alignment & ALIGN_CENTER_H) {
        ASSERT(
            this->size.x <= relative_to->size.x,
            "{} must be <= to {}",
            this->size.x,
            relative_to->size.x);
        this->pos.x = (relative_to->size.x - this->size.x) / 2;
    }

    if (alignment & ALIGN_CENTER_V) {
        ASSERT(
            this->size.y <= relative_to->size.y,
            "{} must be <= to {}",
            this->size.y,
            relative_to->size.y);
        this->pos.y = (relative_to->size.y - this->size.y) / 2;
    }

    return *this;
}

void UIObject::detach() {
    if (!this->parent) {
        return;
    }

    if (auto *container = dynamic_cast<UIContainer*>(this->parent)) {
        container->remove(*this);
    }

    this->parent = nullptr;
}

bool UIObject::is_hovered() const {
    return this->is_enabled()
        && global.ui->mouse_object
        && global.ui->mouse_object->id == this->id;
}

bool UIObject::is_pressed() const {
    return this->is_enabled()
        && this->is_hovered()
        && this->root->click_button()->is_down();
}
