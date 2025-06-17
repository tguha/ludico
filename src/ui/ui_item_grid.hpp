#pragma once

#include "ui/ui_grid.hpp"
#include "ui/ui_item_slot.hpp"

struct ItemStack;

template <typename T>
    requires std::is_base_of_v<UIItemSlot, T>
struct UIItemGrid : public UIGrid<T> {
    using Base = UIGrid<T>;
    using Base::Base;

    inline T &slot(const uvec2 &index) {
        return this->element(index);
    }
};
