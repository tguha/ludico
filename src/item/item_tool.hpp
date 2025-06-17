#pragma once

#include "item/item.hpp"
#include "item/tool_class.hpp"

struct ItemTool : public Item {
    using Base = Item;
    using Base::Base;

    virtual const ToolClass &tool_class() const = 0;

    virtual std::string tool_name() const = 0;

    virtual std::string tool_locale_name() const = 0;

    std::string name() const override {
        return fmt::format("{}_{}", this->tool_name(), this->tool_class().name);
    }

    std::string locale_name() const override {
        return fmt::vformat(
            this->tool_class().prefix_fmt,
            fmt::make_format_args(this->tool_locale_name()));
    }
};
