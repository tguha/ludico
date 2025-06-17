#pragma once

#include "ui/ui_hud.hpp"
#include "ui/ui_item_slot_basic.hpp"
#include "ui/ui_button_arrow.hpp"
#include "ui/ui_item_grid.hpp"

struct EntityPlayer;
struct UIInventoryStorage;
struct UIInventoryExt;

// UIInventory is a bare container for UIInventoryStorage and UIInventoryExt
struct UIInventory
    : public UIContainer,
      public UIEventHandler<UIEventPlayerChange> {
    using Base = UIContainer;

    // SIZE is determined by storage + ext + STORAGE_EXT_SPACING
    static constexpr auto
        SPACING = uvec2(0, 2),
        STORAGE_EXT_SPACING = uvec2(0, 24);

    UIInventoryStorage *storage;
    UIInventoryExt *ext;

    UIInventory();

    UIHUD &hud() const {
        return dynamic_cast<UIHUD&>(*this->parent);
    }

    inline EntityPlayer &player() const {
        return this->hud().player();
    }

    bool is_ghost() const override {
        return true;
    }

    UIInventory &configure(bool enabled = true);

    void reposition() override;

    void tick() override;

    void on_enabled_changed() override;

    // open this inventory for the current player on the HUD - no container
    void open();
};

// base storage in inventory
struct UIInventoryStorage
    : public UIItemGrid<UIItemSlotBasic>,
      public UIEventHandler<UIEventPlayerChange> {
    using Base = UIItemGrid<UIItemSlotBasic>;

    static constexpr auto
        BASE_SLOT_OFFSET = uvec2(0, 0),
        SLOT_OFFSET = UIItemSlotBasic::SIZE + uvec2(1, 1),
        GRID_SIZE = uvec2(8, 2),
        SIZE = (GRID_SIZE * SLOT_OFFSET) - 1u;

    UIInventoryStorage()
        : Base(GRID_SIZE) {}

    UIInventory &inventory() const {
        return dynamic_cast<UIInventory&>(*this->parent);
    }

    void on_event(const UIEventPlayerChange &e) override {
        UIEventHandler<UIEventPlayerChange>::on_event(e);
        this->make_grid();
    }

    UIInventoryStorage &configure(bool enabled = true);

    void render(SpriteBatch &batch, RenderState render_state) override;

    void reposition_element(UIItemSlotBasic &slot, const uvec2 &index) override;

    void create_element(UIItemSlotBasic &slot, const uvec2 &index) override;
};

// top part of inventory menus (containers, player gear, etc.)
struct UIInventoryExt
    : public UIContainer,
      public UIEventHandler<UIEventPlayerChange> {
    using Base = UIContainer;

    static constexpr auto
        PAGE_SIZE =
            uvec2(
                UIInventoryStorage::SIZE.x,
                UIInventoryStorage::SLOT_OFFSET.y * 2 - 1),
        SIZE = PAGE_SIZE + uvec2(0, 2 /* padding */ + 14 /* title */);

    static constexpr usize NO_PAGE = -1;

    // abstract "page", could be container storage, gear page, essences, etc.
    struct Page
        : public UIContainer,
          public UIEventHandler<UIEventPlayerChange> {
        static constexpr auto SIZE = UIInventoryExt::PAGE_SIZE;

        bool is_ghost() const override {
            return true;
        }

        inline UIInventoryExt &ext() const {
            return dynamic_cast<UIInventoryExt&>(*this->parent);
        }

        inline UIInventory &inventory() const {
            return this->ext().inventory();
        }

        inline UIHUD &hud() const {
            return this->inventory().hud();
        }

        virtual std::string title() const {
            return "MISSING";
        }
    };

    // paginate buttons near title
    struct PaginateButton : public UIButtonArrow {
        using Base = UIButtonArrow;

        UIInventoryExt &ext;

        PaginateButton(
            UIInventoryExt &ext,
            UIButtonArrow::Type type)
            : Base(type),
              ext(ext) {}

        PaginateButton &configure(bool enabled = true) {
            Base::configure(enabled);
            return *this;
        }
    };

    // paginate buttons, only enabled if there are multiple pages
    std::array<PaginateButton*, 2> paginate_buttons;

    UIInventoryExt();

    UIInventory &inventory() const {
        return dynamic_cast<UIInventory&>(*this->parent);
    }

    UIInventoryExt &configure(bool enabled = true);

    bool is_ghost() const override {
        return true;
    }

    void on_enabled_changed() override;

    void reposition() override;

    void render(SpriteBatch &batch, RenderState render_state) override;

    // adds a new page to inventory ext
    // returns (page index, page ref)
    template <typename T>
        requires std::is_base_of_v<Page, T>
    std::tuple<usize, T&> add_page(std::unique_ptr<T> &&page) {
        auto &ref = this->add(std::move(page));
        this->pages.push_back(&ref);
        return std::tuple<usize, T&>(this->pages.size() - 1, ref);
    }

    // remove all existing inventory ext pages
    void reset_pages() {
        for (auto *page : this->pages) {
            this->remove(*page);
        }
        this->pages.clear();
        this->page_index = NO_PAGE;
    }

    void set_page(usize page_index) {
        const auto old = this->page_index;
        this->page_index = page_index;

        if (this->page_index != old) {
            if (old != NO_PAGE) {
                this->pages[old]->set_enabled(false);
            }

            this->pages[this->page_index]->set_enabled(true);
            this->update_paginate_buttons();
        }
    }

    Page *page() const {
        return this->page(this->page_index);
    }

    Page *page(usize index) const {
        if (index == NO_PAGE
                || index < 0
                || index >= this->pages.size()) {
            return nullptr;
        }

        return this->pages[index];
    }

private:
    // update visibility of pagination buttons:
    void update_paginate_buttons();

    // index of current page in pages, set to 0 on construction
    usize page_index = NO_PAGE;

    std::vector<Page*> pages;
};
