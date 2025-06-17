#include "ui/ui_container.hpp"
#include "global.hpp"
#include "util/collections.hpp"

UIContainer &UIContainer::remove(UIObject &object) {
    for (
        auto it = this->children.begin();
        it != this->children.end();
        it++) {
        if ((*it)->id == object.id) {
            this->children.erase(it);
            return *this;
        }
    }

    ASSERT(false, "no object with ID {}", object.id);
    return *this;
}

bool UIContainer::contains(UIObject &object) {
    return std::find_if(
        this->begin(), this->end(),
        [&](auto &child) {
            return child->id == object.id;
        }) != this->end();
}

void UIContainer::update() {
    for (auto &obj : *this) {
        if (obj->is_enabled()) {
            obj->update();
        }
    }
}

void UIContainer::tick() {
    for (auto &obj : *this) {
        if (obj->is_enabled()) {
            obj->tick();
        }
    }
}

void UIContainer::render(SpriteBatch &batch, RenderState render_state) {
    // copy and sort children
    std::vector<UIObject*> objects;
    collections::copy_to(
        this->children,
        objects,
        [](auto &ptr) {
            return ptr.get();
        });

    std::sort(
        objects.begin(), objects.end(),
        [](const auto &a, const auto &b) {
            return a->z < b->z;
        });

    for (auto &obj : objects) {
        if (obj->is_enabled()) {
            obj->render(batch, render_state);
        }
    }
}

void UIContainer::render_debug(
    SpriteBatch &batch_s,
    QuadBatch &batch_q,
    RenderState render_state) {
    Base::render_debug(batch_s, batch_q, render_state);
    for (auto &obj : *this) {
        if (obj->is_enabled()) {
            obj->render_debug(batch_s, batch_q, render_state);
        }
    }
}

void UIContainer::reposition() {
    for (auto &obj : *this) {
        obj->reposition();
    }
}

bool UIContainer::click(const ivec2 &pos, bool is_release) {
    const auto obj = this->topmost(pos, true);

    if (obj && obj->is_enabled()) {
        return obj->click(pos - obj->pos, is_release);
    }

    return false;
}

bool UIContainer::hover(const ivec2 &pos) {
    const auto obj = this->topmost(pos, true);

    if (obj && obj->is_enabled()) {
        return obj->hover(pos - obj->pos);
    }

    // containers do not hover in-and-of themselves
    return false;
}

UIObject *UIContainer::topmost(
    const ivec2 &pos,
    bool include_ghost) const {
    for (isize i = this->count() - 1; i >= 0; i--) {
        auto &child = (*this)[i];
        if (child.is_enabled()
                && (include_ghost || !child.is_ghost())
                && child.aabb().contains(pos)) {
            return &child;
        }
    }

    return nullptr;
}

UIObject *UIContainer::topmost_recursive(
    const ivec2 &pos,
    bool include_ghost) const {
    for (isize i = this->count() - 1; i >= 0; i--) {
        auto &child = (*this)[i];
        if (child.is_enabled() && child.aabb().contains(pos)) {
            if (auto *container = dynamic_cast<UIContainer*>(&child)) {
                if (const auto res =
                        container->topmost_recursive(pos - container->pos)) {
                    return res;
                }
            }

            if (!include_ghost && child.is_ghost()) {
                return nullptr;
            }

            return &child;
        }
    }

    return nullptr;
}
