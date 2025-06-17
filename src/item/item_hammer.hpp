#pragma once

#include "item/item_tool.hpp"
#include "locale.hpp"

struct ItemHammer : public ItemTool {
    using Base = ItemTool;
    using Base::Base;

    std::string tool_name() const override {
        return "hammer";
    }

    std::string tool_locale_name() const override {
        return Locale::instance()["item:hammer"];
    }

    HoldType hold_type() const override {
        return HoldType::ATTACHMENT_FLIPPED;
    }

    vec3 hold_offset() const override {
        return vec3(-1, 1, 0);
    }
};

struct ItemHammerCopper
    : public ItemHammer,
      public ItemType<ItemHammerCopper> {
    using Base = ItemHammer;
    using Base::Base;

    const ToolClass &tool_class() const override {
        return ToolClass::get(ToolClass::CLASS_COPPER);
    }

    const ItemIcon &icon() const override;
};
