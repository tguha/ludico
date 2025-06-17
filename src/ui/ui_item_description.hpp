#pragma once

#include "ui/ui_fade_helper.hpp"
#include "ui/ui_event_player_change.hpp"
#include "item/item_stack.hpp"
#include "item/item.hpp"
#include "util/collections.hpp"

struct UIItemDescription
    : public UIObject,
      public UIEventHandler<UIEventPlayerChange> {
    using Base = UIObject;

    // extra lines for a description
    struct Line {
        std::string text;
        vec4 color;
    };

    struct Entry {
        usize count;
        std::string name;
        usize hold_ticks = 0;

        // extra lines
        std::vector<Line> lines;

        Entry() = default;

        Entry(usize count, const std::string &name, usize hold_ticks = 0)
            : count(count),
              name(name),
              hold_ticks(hold_ticks) {}

        // intentionally non-explicit
        Entry(const ItemStack &stack, usize hold_ticks = 0)
            : count(stack.size),
              name("(no name)"),
              hold_ticks(hold_ticks) {
            this->name = stack.item().locale_name();

            collections::copy_to(
                stack.item().description(stack),
                this->lines,
                [](const auto &t){
                    const auto &[s, c] = t;
                    return Line { s, c };
                });
        }
    };

    static const inline auto
        BOX_NAME = AABB2u({ 120, 10 }),
        BOX_COUNT = AABB2u({ 24, 10 }),
        BOX_LINES = AABB2u({ 120, 20 });

    static const inline auto
        SIZE =
            AABB2u::merge(
                BOX_NAME,
                BOX_COUNT
                    .translate({ 0, BOX_NAME.size().y }),
                BOX_LINES
                    .translate({ 0, BOX_NAME.size().y + BOX_LINES.size().y }))
                .size();

    static constexpr auto
        SPACING = ivec2(-2, 2);

    UIItemDescription() = default;

    bool is_set_this_tick() const {
        return this->fade.is_set_this_tick();
    }

    void set(Entry &&entry) {
        this->fade.set(std::move(entry), entry.hold_ticks);
    }

    void on_event(const UIEventPlayerChange &event) override {
        UIEventHandler<UIEventPlayerChange>::on_event(event);
        this->fade = UIFadeHelper<Entry>(FADE_TIME);
    }

    UIItemDescription &configure(bool enabled = true) {
        Base::configure(SIZE, enabled);
        this->reposition();
        return *this;
    }

    bool is_ghost() const override { return true; }

    void reposition() override;

    void render(SpriteBatch &batch, RenderState render_state) override;

private:
    static constexpr auto FADE_TIME = 80;

    UIFadeHelper<Entry> fade = UIFadeHelper<Entry>(FADE_TIME);
};
