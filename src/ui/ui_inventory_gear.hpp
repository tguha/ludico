#pragma once

#include "ui/ui_inventory.hpp"
#include "ui/ui_battery_grid.hpp"

struct UIInventoryGear : public UIInventoryExt::Page {
    using Base = UIInventoryExt::Page;

    struct BasicGrid
        : public UIItemGrid<UIItemSlotBasic>,
          public UIEventHandler<UIEventPlayerChange> {
        using Base = UIItemGrid<UIItemSlotBasic>;
        using Base::Base;

        UIInventoryGear &gear() const {
            return dynamic_cast<UIInventoryGear&>(*this->parent);
        }

        void reposition_element(
            UIItemSlotBasic &slot,
            const uvec2 &index) override {
            slot.pos = index * (UIItemSlotBasic::SIZE + 1u);
        }

        void on_event(const UIEventPlayerChange &e) override {
            UIEventHandler<UIEventPlayerChange>::on_event(e);
            this->make_grid();
        }
    };

    struct BatteryGrid : public UIBatteryGrid {
        using Base = UIBatteryGrid;
        using Base::Base;

        void reposition() override {
            Base::reposition();
            this->align(UIObject::ALIGN_LEFT | UIObject::ALIGN_CENTER_V);
        }
    };

    struct LogicBoardSlot : public UIItemSlotBasic {
        using Base = UIItemSlotBasic;

        UIInventoryGear &gear() const {
            return dynamic_cast<UIInventoryGear&>(*this->parent);
        }

        LogicBoardSlot &configure(bool enabled = true);

        void reposition() override {
            this->align(UIObject::ALIGN_LEFT | UIObject::ALIGN_CENTER_V);
            this->pos.x += BatteryGrid::SIZE.x + 8;
        }
    };

    struct ToolsGrid : public BasicGrid {
        using Base = BasicGrid;

        static constexpr auto
            GRID_SIZE = uvec2(2, 1),
            SIZE = ((UIItemSlotBasic::SIZE + 1u) * GRID_SIZE) - 1u;

        ToolsGrid() : Base(GRID_SIZE) {}

        ToolsGrid &configure(bool enabled = true);

        void reposition() override {
            this->align(UIObject::ALIGN_RIGHT | UIObject::ALIGN_CENTER_V);
        }

        void create_element(
            UIItemSlotBasic &slot,
            const uvec2 &index) override {
            slot.slot_ref =
                this->gear().inventory().player().inventory()->tools()[
                    index.y * GRID_SIZE.x + index.x];
        }
    };

    static constexpr auto
        SPACING = uvec2(UIItemSlotBasic::SIZE.x + 1, 0),
        SIZE =
            uvec2(
                BatteryGrid::SIZE.x
                    + 8
                    + LogicBoardSlot::SIZE.x
                    + 7
                    + ToolsGrid::SIZE.x,
                math::max(BatteryGrid::SIZE.y, ToolsGrid::SIZE.y));

    LogicBoardSlot *logic_board_slot;
    BatteryGrid *battery_grid;
    ToolsGrid *tools_grid;

    explicit UIInventoryGear(
        EntityPlayer &player) {
        this->logic_board_slot =
            &this->add(std::make_unique<LogicBoardSlot>());
        this->battery_grid =
            &this->add(std::make_unique<BatteryGrid>(player));
        this->tools_grid =
            &this->add(std::make_unique<ToolsGrid>());
    }

    bool is_ghost() const override {
        return true;
    }

    std::string title() const override {
        return "GEAR";
    }

    UIInventoryGear &configure(bool enabled = true) {
        UIObject::configure(SIZE, enabled);
        this->logic_board_slot->configure(true);
        this->battery_grid->configure(true);
        this->tools_grid->configure(true);
        this->reposition();
        return *this;
    }

    void reposition() override {
        Base::reposition();
        this->align(UIObject::ALIGN_CENTER_H | UIObject::ALIGN_BOTTOM);
    }
};
