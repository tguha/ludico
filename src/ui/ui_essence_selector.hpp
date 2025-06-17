#pragma once

#include "ui/ui_container.hpp"
#include "ui/ui_essence_container.hpp"
#include "ui/ui_button_arrow.hpp"

struct UIEssenceSelector : public UIContainer {
    using Base = UIContainer;

    static constexpr auto
        SIZE =
            uvec2(
                std::max({
                    UIEssenceContainer::SIZE.x,
                    UIButtonArrow::size_of(UIButtonArrow::UP).x,
                    UIButtonArrow::size_of(UIButtonArrow::DOWN).x
                }),
                UIEssenceContainer::SIZE.y
                    + UIButtonArrow::size_of(UIButtonArrow::UP).y
                    + 0
                    + UIButtonArrow::size_of(UIButtonArrow::DOWN).y
                    + 0);

    UIEssenceContainer *container;
    std::array<UIButtonArrow*, 2> buttons;

    explicit UIEssenceSelector(
        std::unique_ptr<UIEssenceContainer> &&container) {
        this->container = &this->add(std::move(container));

        for (usize i = 0; i < 2; i++) {
            auto &button =
                this->add(
                    std::make_unique<UIButtonArrow>(
                        i == 0 ? UIButtonArrow::UP : UIButtonArrow::DOWN));
            this->buttons[i] = &button;
        }
    }

    UIEssenceSelector &configure(bool enabled = false) {
        Base::configure(SIZE, enabled);
        this->container->configure(true);
        this->container->align(
            UIObject::ALIGN_CENTER_H
            | UIObject::ALIGN_CENTER_V);
        for (usize i = 0; i < 2; i++) {
            this->buttons[i]->configure(true);
            this->buttons[i]->align(
                UIObject::ALIGN_CENTER_H
                | (this->buttons[i]->type == UIButtonArrow::UP ?
                    UIObject::ALIGN_TOP
                    : UIObject::ALIGN_BOTTOM));
        }
        return *this;
    }

    bool is_ghost() const override {
        return true;
    }
};
