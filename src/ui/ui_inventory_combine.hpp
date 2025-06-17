#pragma once

#include "ui/ui_inventory.hpp"
#include "ui/ui_essence_container.hpp"
#include "ui/ui_essence_selector.hpp"
#include "ui/ui_icon.hpp"

struct UIInventoryCombine : public UIInventoryExt::Page {
    using Base = UIInventoryExt::Page;

    static constexpr auto
        CONTAINER_OFFSET_CRAFT = 0,
        CONTAINER_OFFSET_RESULT = 4;

    // 2x2 ingredient grid
    struct IngredientsGrid : public UIItemGrid<UIItemSlotBasic> {
        using Base = UIItemGrid<UIItemSlotBasic>;

        static constexpr auto
            GRID_SIZE = uvec2(2, 2),
            SIZE = (GRID_SIZE * (UIItemSlotBasic::SIZE + 1u)) - 1u;

        IngredientsGrid()
            : Base(GRID_SIZE) {}

        UIInventoryCombine &craft() const {
            return dynamic_cast<UIInventoryCombine&>(*this->parent);
        }

        IngredientsGrid &configure(bool enabled = true) {
            Base::configure(SIZE, enabled);
            this->reposition();
            for (auto *slot : this->elements) {
                slot->color = UIItemSlotBasic::SLOT_COLOR_RED;
            }
            return *this;
        }

        void reposition() override {
            this->align(UIObject::ALIGN_LEFT | UIObject::ALIGN_CENTER_V);
        }

        void reposition_element(
            UIItemSlotBasic &slot,
            const uvec2 &index) override {
            slot.pos = index * (UIItemSlotBasic::SIZE + 1u);
        }

        void create_element(
            UIItemSlotBasic &slot,
            const uvec2 &index) override {
            slot.slot_ref =
                &this->craft().container.slots[
                    CONTAINER_OFFSET_CRAFT
                        + (index.y * this->grid_size.x + index.x)];
        }
    };

    struct EssenceSelector : public UIEssenceSelector {
        using Base = UIEssenceSelector;
        using Base::Base;

        UIInventoryCombine &craft() const {
            return dynamic_cast<UIInventoryCombine&>(*this->parent);
        }

        UIEssenceSelector &configure(bool enabled = true) {
            Base::configure(enabled);
            this->reposition();
            return *this;
        }

        void reposition() override {
            this->align(UIObject::ALIGN_LEFT | UIObject::ALIGN_CENTER_V);
            this->pos.x +=
                IngredientsGrid::SIZE.x
                    + PADDING_BETWEEN
                    + CombineIcon::SIZE.x
                    + PADDING_BETWEEN;
        }
    };

    // combine output slot
    struct ResultSlot : public UIItemSlotBasic {
        using Base = UIItemSlotBasic;

        UIInventoryCombine &craft() const {
            return dynamic_cast<UIInventoryCombine&>(*this->parent);
        }

        ResultSlot &configure(bool enabled = true) {
            Base::configure(enabled);
            this->color = UIItemSlotBasic::SLOT_COLOR_RED;
            this->slot_ref =
                &this->craft().container.slots[CONTAINER_OFFSET_RESULT];
            this->reposition();
            return *this;
        }

        void reposition() override {
            this->align(UIObject::ALIGN_RIGHT | UIObject::ALIGN_CENTER_V);
        }
    };

    // combine battery/progress indicator icon
    struct BatteryIndicator : public UIObject {
        using Base = UIObject;

        // NOTE: must match with sprite size
        static constexpr auto SIZE = uvec2(13, 5);

        BatteryIndicator &configure(bool enabled = true) {
            Base::configure(SIZE, enabled);
            this->reposition();
            return *this;
        }

        void reposition() override {
            this->align(UIObject::ALIGN_RIGHT | UIObject::ALIGN_CENTER_V);
            this->pos.x -=
                ResultSlot::SIZE.x
                    + PADDING_BETWEEN;
        }

        void render(SpriteBatch &batch, RenderState render_state) override;
    };

    struct CombineIcon : public UIIcon {
        using Base = UIIcon;

        static constexpr auto SIZE = uvec2(8, 8);

        CombineIcon();

        void reposition() override {
            this->align(UIObject::ALIGN_LEFT | UIObject::ALIGN_CENTER_V);
            this->pos.x +=
                IngredientsGrid::SIZE.x
                    + PADDING_BETWEEN;
        }
    };

    static constexpr auto
        PADDING_BETWEEN = 2u,
        PADDING_VERTICAL = 1u;

    static constexpr auto
        SIZE =
            uvec2(
                IngredientsGrid::SIZE.x
                    + PADDING_BETWEEN /* padding: grid - x */
                    + CombineIcon::SIZE.x /* x */
                    + PADDING_BETWEEN /* padding: x - essence */
                    + EssenceSelector::SIZE.x
                    + PADDING_BETWEEN /* padding: essence - battery */
                    + BatteryIndicator::SIZE.x
                    + PADDING_BETWEEN /* padding: battery - result */
                    + ResultSlot::SIZE.x,
                IngredientsGrid::SIZE.y
                    + PADDING_VERTICAL /* padding */);

    // TODO: this should not need a level
    ItemContainer container = ItemContainer::make_basic(5);

    IngredientsGrid *ingredients_grid;
    EssenceSelector *essence_selector;
    BatteryIndicator *battery_indicator;
    ResultSlot *result_slot;
    CombineIcon *combine_icon;

    UIInventoryCombine() {
        this->ingredients_grid =
            &this->add(std::make_unique<IngredientsGrid>());
        this->essence_selector =
            &this->add(
                std::make_unique<EssenceSelector>(
                    std::make_unique<UIEssenceContainer>()));
        this->battery_indicator =
            &this->add(std::make_unique<BatteryIndicator>());
        this->result_slot =
            &this->add(std::make_unique<ResultSlot>());
        this->combine_icon =
            &this->add(std::make_unique<CombineIcon>());
    }

    bool is_ghost() const override {
        return true;
    }

    std::string title() const override {
        return "COMBINE";
    }

    UIInventoryCombine &configure(bool enabled = true) {
        Base::configure(SIZE, enabled);
        this->ingredients_grid->configure();
        this->essence_selector->configure();
        this->battery_indicator->configure();
        this->result_slot->configure();
        this->combine_icon->configure();
        this->reposition();
        return *this;
    }

    void reposition() override {
        Base::reposition();
        this->align(UIObject::ALIGN_CENTER_H | UIObject::ALIGN_BOTTOM);
    }
};
