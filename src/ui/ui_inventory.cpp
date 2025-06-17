#include "ui/ui_inventory.hpp"
#include "ui/ui_stats.hpp"
#include "ui/ui_hotbar.hpp"
#include "ui/ui_inventory_gear.hpp"
#include "ui/ui_inventory_combine.hpp"
#include "ui/ui_inventory_essence.hpp"
#include "entity/entity_player.hpp"
#include "gfx/sprite_resource.hpp"
#include "gfx/game_camera.hpp"
#include "gfx/font.hpp"
#include "gfx/renderer.hpp"
#include "state/state_game.hpp"

// grid size must fit evenly with player inventory
static_assert(
    (UIInventoryStorage::GRID_SIZE.x * UIInventoryStorage::GRID_SIZE.y)
        + UIHotbar::NUM_ENTRIES
            == EntityPlayerInventory::SIZE_NON_HIDDEN);

static const auto spritesheet =
    SpritesheetResource("ui_inventory", "ui/inventory.png", { 16, 16 });

static const auto sprite_title =
    SpriteResource(
        [](const auto &_) {
            return
                spritesheet[AABB2u({ 0, 0 }, { 3, 0 })]
                    .subpixels(AABB2u({ 0, 0 }, { 64 - 1, 14 - 1 }));
        });

UIInventory::UIInventory()
    : Base() {
    this->storage = &this->add(std::make_unique<UIInventoryStorage>());
    this->ext = &this->add(std::make_unique<UIInventoryExt>());
}

UIInventory &UIInventory::configure(bool enabled) {
    Base::configure(
        uvec2(
            math::max(
                UIInventoryStorage::SIZE.x,
                UIInventoryExt::SIZE.x),
            UIInventoryStorage::SIZE.y
                + UIInventoryExt::SIZE.y
                + STORAGE_EXT_SPACING.y),
        enabled);
    this->storage->configure(enabled);
    this->ext->configure(enabled);
    this->reposition();
    return *this;
}

void UIInventory::reposition() {
    Base::reposition();

    // position directly above stats
    this->align(UIObject::ALIGN_CENTER_H | UIObject::ALIGN_BOTTOM);
    const auto &stats = *this->hud().stats;
    this->pos.y = stats.pos.y;
    this->pos.y += stats.size.y;
    this->pos += SPACING;
}

void UIInventory::tick() {
    Base::tick();

    if (this->is_enabled()) {
        global.game->camera->offset.target = vec2(0, 12);
    }
}

void UIInventory::on_enabled_changed() {
    if (!this->is_enabled()) {
        global.game->camera->offset.target = vec2(0, 0);
    }

    this->storage->set_enabled(this->is_enabled());
    this->ext->set_enabled(this->is_enabled());
}

void UIInventory::open() {
    this->ext->reset_pages();
    // add default inventory pages
    auto [i_gear, gear] =
        this->ext->add_page(
            std::make_unique<UIInventoryGear>(this->player()));
    gear.configure(false);

    auto [i_combine, combine] =
        this->ext->add_page(
            std::make_unique<UIInventoryCombine>());
    combine.configure(false);


    auto [i_essence, essence] =
        this->ext->add_page(
            std::make_unique<UIInventoryEssence>());
    essence.configure(false);

    this->ext->set_page(0);

    // show inventory
    this->set_enabled(true);
}

UIInventoryStorage &UIInventoryStorage::configure(bool enabled) {
    Base::configure(UIInventoryStorage::SIZE, enabled);
    return *this;
}

void UIInventoryStorage::render(SpriteBatch &batch, RenderState render_state) {
    Base::render(batch, render_state);
}

void UIInventoryStorage::reposition_element(
    UIItemSlotBasic &slot,
    const uvec2 &index) {
    slot.pos = BASE_SLOT_OFFSET + (SLOT_OFFSET * index);
}

void UIInventoryStorage::create_element(
    UIItemSlotBasic &slot,
    const uvec2 &index) {
    auto &container = *this->inventory().hud().player().inventory();

    // inventory starts after hotbar slots
    slot.slot_ref =
        &container[UIHotbar::NUM_ENTRIES + (index.y * GRID_SIZE.x + index.x)];
}

UIInventoryExt::UIInventoryExt()
    : Base() {
    for (const auto &[type, i] :
            types::make_array(
                std::make_tuple(UIButtonArrow::LEFT, 0),
                std::make_tuple(UIButtonArrow::RIGHT, 1))) {
        this->paginate_buttons[i] =
            &this->add(
                std::make_unique<PaginateButton>(
                    *this,
                    type));

        auto &button = *this->paginate_buttons[i];

        button.on_release =
            [&button](const auto &_) {
                const auto new_index =
                    button.ext.page_index +
                        isize(button.type == UIButtonArrow::LEFT ? -1 : 1);

                if (new_index >= 0
                        && new_index < button.ext.pages.size()) {
                    button.ext.set_page(new_index);
                }
            };

        button.configure(true);
    }
}

// get aabb of UIInventoryExt title box
static AABB2i get_aabb_title(UIInventoryExt &ext) {
    return
        AABB2i(ivec2(sprite_title->size()))
            .translate({
                (ext.size.x - sprite_title->size().x) / 2,
                ext.size.y - sprite_title->size().y
            });
}

UIInventoryExt &UIInventoryExt::configure(bool enabled) {
    Base::configure(SIZE, enabled);
    this->reposition();
    return *this;
}

void UIInventoryExt::on_enabled_changed() {
    Base::on_enabled_changed();

    for (auto *b : this->paginate_buttons) {
        b->set_enabled(!this->pages.empty());
    }

    this->page_index = 0;
    this->update_paginate_buttons();
}

void UIInventoryExt::update_paginate_buttons() {
    this->paginate_buttons[0]
        ->set_enabled(
            this->pages.size() > 1
                && this->page_index != 0);
    this->paginate_buttons[1]
        ->set_enabled(
            this->pages.size() > 1
                && this->page_index != this->pages.size() - 1);
}

void UIInventoryExt::reposition() {
    Base::reposition();

    // position above storage
    this->pos =
        uvec2(
            0,
            UIInventoryStorage::SIZE.y
                + UIInventory::STORAGE_EXT_SPACING.y);

    const auto aabb_title = get_aabb_title(*this);
    for (auto *button : this->paginate_buttons) {
        button->pos =
            ivec2(
                button->type == UIButtonArrow::LEFT ?
                    (aabb_title.min.x - button->size.x - 1)
                    : (aabb_title.max.x + 1),
                aabb_title.min.y + 1);
    }
}

void UIInventoryExt::render(SpriteBatch &batch, RenderState render_state) {
    Base::render(batch, render_state);

    auto &renderer = Renderer::get();
    StackAllocator<4096> allocator;

    // render title
    const auto aabb_title_absolute =
        get_aabb_title(*this)
            .translate(this->pos_absolute());
    batch.push(*sprite_title, aabb_title_absolute.min);

    const auto title = this->page() ? this->page()->title() : "NOTHING";
    renderer.font->render(
        batch,
        std::span<const Font::PosString>(
            renderer.font->fit(
                &allocator,
                title,
                AABB2u(aabb_title_absolute),
                Font::ALIGN_CENTER_H | Font::ALIGN_CENTER_V,
                0)),
        vec4(vec3(0.8f), 1.0f),
        Font::Flags::DOUBLED);
}
