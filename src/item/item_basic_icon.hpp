#pragma once

#include "item/item.hpp"
#include "item/item_icon.hpp"

struct ItemBasicIcon : virtual public Item {
    using Base = Item;
    using Base::Base;

    virtual TextureArea sprite() const = 0;

    const ItemIconBasic &icon() const override {
        if (!this->_icon) {
            this->_icon = ItemIconBasic(this->sprite());
        }

        return *this->_icon;
    }

private:
    mutable std::optional<ItemIconBasic> _icon;
};
