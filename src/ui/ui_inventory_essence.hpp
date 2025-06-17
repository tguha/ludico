#pragma once

#include "ui/ui_inventory.hpp"
#include "ui/ui_essence_container.hpp"
#include "essence/essence.hpp"
#include "entity/entity_player.hpp"

struct UIInventoryEssence : public UIInventoryExt::Page {
    using Base = UIInventoryExt::Page;

    struct EssenceContainer : public UIEssenceContainer {
        using Base = UIEssenceContainer;
        using Base::Base;

        Essence kind;

        UIInventoryEssence &essence() const {
            return dynamic_cast<UIInventoryEssence&>(*this->parent->parent);
        }

        EssenceHolder &holder() const override {
            return this->essence().inventory().player().essences[this->kind];
        }
    };

    struct EssenceGrid : public UIGrid<EssenceContainer> {
        using Base = UIGrid<EssenceContainer>;
        using Base::Base;

        // essences hash based on last time grid was updated
        u64 hash_essences = 0;

        // try to update grid if essences have updated
        void update_grid() {
            const auto &essences =
                this->essence().inventory().player().essences;
            const auto hash_current = hash(essences.keys());

            // only update if hash is different
            if (this->hash_essences == hash_current) {
                return;
            }

            this->hash_essences = hash_current;

            const auto grid_size = uvec2(essences.size(), 1);
            this->size =
                essences.size() == 0 ?
                uvec2(0, 0)
                : ((grid_size * (EssenceContainer::SIZE + 1u)) - 1u);
            this->make_grid(grid_size);
        }

        EssenceGrid &configure(bool enabled = true) {
            Base::configure(uvec2(0), enabled);
            this->update_grid();
            return *this;
        }

        UIInventoryEssence &essence() const {
            return dynamic_cast<UIInventoryEssence&>(*this->parent);
        }

        void reposition() override {
            Base::reposition();
            this->align(UIObject::ALIGN_CENTER_H | UIObject::ALIGN_CENTER_V);
        }

        void reposition_element(
            EssenceContainer &obj,
            const uvec2 &index) override {
            obj.pos = index * (EssenceContainer::SIZE + 1u);
        }

        void create_element(
            EssenceContainer &obj,
            const uvec2 &index) override {
            const auto &essences =
                this->essence().inventory().player().essences;
            obj.kind = essences.keys().nth(index.x);
        }

        void tick() override {
            Base::tick();
            this->update_grid();
        }
    };

    static constexpr auto NUM_ESSENCES = 6;
    static constexpr auto SIZE = UIInventoryExt::PAGE_SIZE;

    EssenceGrid *essence_grid;

    void on_add() override {
        Base::on_add();
        this->essence_grid =
            &this->add(std::make_unique<EssenceGrid>());
    }

    bool is_ghost() const override {
        return true;
    }

    std::string title() const override {
        return "ESSENCE";
    }

    UIInventoryEssence &configure(bool enabled = true) {
        Base::configure(SIZE, enabled);
        this->essence_grid->configure();
        this->reposition();
        return *this;
    }

    void reposition() override {
        Base::reposition();
        this->align(UIObject::ALIGN_CENTER_H | UIObject::ALIGN_BOTTOM);
    }
};
