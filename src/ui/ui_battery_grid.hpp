#pragma once

#include "ui/ui_container.hpp"
#include "ui/ui_essence_selector.hpp"
#include "ui/ui_essence_container.hpp"
#include "ui/ui_item_slot_basic.hpp"
#include "ui/ui_item_grid.hpp"
#include "ui/ui_event_player_change.hpp"
#include "state/state_game.hpp"
#include "entity/entity_player.hpp"
#include "global.hpp"

struct UIBatteryGrid
    : public UIContainer,
      public UIEventHandler<UIEventPlayerChange> {
    using Base = UIContainer;

    static constexpr auto
        Z_EXTRAS = SpriteBatch::Z_DEFAULT + 1;

    struct Grid
        : public UIItemGrid<UIItemSlotBasic>,
          public UIEventHandler<UIEventPlayerChange> {
        using Base = UIItemGrid<UIItemSlotBasic>;

        static constexpr auto
            GRID_SIZE = uvec2(2, 2),
            SIZE = ((UIItemSlotBasic::SIZE + 1u) * GRID_SIZE) - 1u;

        Grid()
            : Base(GRID_SIZE) {}

        UIBatteryGrid &battery_grid() const {
            return dynamic_cast<UIBatteryGrid&>(*this->parent);
        }

        void on_event(const UIEventPlayerChange &e) override {
            UIEventHandler<UIEventPlayerChange>::on_event(e);
            this->make_grid();
        }

        Grid &configure(bool enabled = true);

        void reposition_element(
            UIItemSlotBasic &slot,
            const uvec2 &index) override {
            slot.pos = index * (UIItemSlotBasic::SIZE + 1u);
        }

        void create_element(
            UIItemSlotBasic &slot,
            const uvec2 &index) override {
            slot.slot_ref =
                this->battery_grid().player->inventory()->batteries()[
                    index.y * this->grid_size.x + index.x];
        }
    };

    struct GeneratorSlot : public UIItemSlotBasic {
        using Base = UIItemSlotBasic;
        using Base::Base;

        UIBatteryGrid &battery_grid() const {
            return dynamic_cast<UIBatteryGrid&>(*this->parent);
        }

        GeneratorSlot &configure(bool enabled = true) {
            Base::configure(enabled);
            this->slot_ref =
                &this->battery_grid().player->inventory()->generator();
            return *this;
        }
    };

    static constexpr auto
        SIZE =
            uvec2(
                UIEssenceSelector::SIZE.x
                    + 1
                    + GeneratorSlot::SIZE.x
                    + 1
                    + Grid::SIZE.x,
                math::max(
                    UIEssenceSelector::SIZE.y,
                    math::max(
                        GeneratorSlot::SIZE.y,
                        Grid::SIZE.y)));

    EntityPlayerRef player;
    UIEssenceSelector *selector;
    GeneratorSlot *generator_slot;
    Grid *grid;

    explicit UIBatteryGrid(EntityPlayer &player)
        : player(player) {
        this->selector =
            &this->add(
                std::make_unique<UIEssenceSelector>(
                    std::make_unique<UIEssenceContainer>()));
        this->generator_slot =
            &this->add(
                std::make_unique<GeneratorSlot>());
        this->grid =
            &this->add(std::make_unique<Grid>());
    }

    void on_event(const UIEventPlayerChange &e) override {
        UIEventHandler<UIEventPlayerChange>::on_event(e);
        this->player = e.player.as<EntityPlayer>();
    }

    UIBatteryGrid &configure(bool enabled = true) {
        Base::configure(SIZE, enabled);
        this->selector->configure(true);
        this->selector->align(
            UIObject::ALIGN_LEFT | UIObject::ALIGN_CENTER_V);

        this->generator_slot->configure(true);
        this->generator_slot->align(
            UIObject::ALIGN_LEFT | UIObject::ALIGN_CENTER_V);
        this->generator_slot->pos.x += UIEssenceSelector::SIZE.x + 1;

        this->grid->configure(true);
        this->grid->align(
            UIObject::ALIGN_RIGHT | UIObject::ALIGN_CENTER_V);
        this->grid->pos.y += 1;

        return *this;
    }

    void render(SpriteBatch &batch, RenderState render_state) override;
};
