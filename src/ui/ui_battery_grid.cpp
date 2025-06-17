#include "ui/ui_battery_grid.hpp"
#include "gfx/sprite_resource.hpp"

static const auto spritesheet =
    SpritesheetResource("ui_power", "ui/power.png", { 16, 16 });

static constexpr auto EXTRAS_OFFSET =
    ivec2(UIEssenceSelector::SIZE.x - 1, 1);

static const auto sprite_extras =
    SpriteResource(
        [](const auto &_) {
            return
                spritesheet[AABB2u({ 0, 0 }, { 2, 1 })]
                    .subpixels(AABB2u({ 0, 0 }, { 37 - 1, 23 - 1 }));
        });

struct ExtraInfo {
    ivec2 offset;
    uvec2 size;
    std::vector<uvec2> enabled_with;
};

static const std::array<ExtraInfo, 7> extras = {
        // to selector
        ExtraInfo {
            { 0, 9 },
            { 3, 6 },
            {}
        },
        // from generator, top
        {
            { 6, 17 },
            { 10, 6 },
            {{ 0, 1 }}
        },
        // from generator, bottom
        {
            { 6, 1 },
            { 10, 6 },
            {{ 0, 0 }}
        },
        // top left -> top right
        {
            { 26, 18 },
            { 3, 5 },
            {{ 0, 1 }, { 1, 1 }}
        },
        // top left -> bottom left
        {
            { 18, 11 },
            { 5, 3 },
            {{ 0, 1 }, { 0, 0 }}
        },
        // top right -> bottom right
        {
            { 32, 11 },
            { 5, 3 },
            {{ 1, 1 }, { 1, 0 }}
        },
        // bottom left -> bottom right
        {
            { 25, 1 },
            { 3, 6 },
            {{ 0, 0 }, { 1, 0 }}
        },
    };

UIBatteryGrid::Grid &UIBatteryGrid::Grid::configure(bool enabled) {
    Base::configure(SIZE, enabled);
    for (auto *slot : this->elements) {
        slot->overlay =
            spritesheet[{ 3, 0 }].subpixels(AABB2u({ 8 - 1, 8 - 1 }));
    }
    return *this;
}

void UIBatteryGrid::render(SpriteBatch &batch, RenderState render_state) {
    Base::render(batch, render_state);

    // draw extras
    for (const auto &e : extras) {
        // TODO: draw conditionally
        /* auto draw = true; */
        /* for (const auto &i : e.enabled_with) { */
        /* } */

        batch.push(
            sprite_extras->subpixels(
                AABB2u(e.size).translate(uvec2(e.offset))),
            this->pos_absolute() + EXTRAS_OFFSET + e.offset,
            vec4(0.0f),
            1.0f,
            Z_EXTRAS);
    }
}
