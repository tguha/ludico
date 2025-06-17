#include "ui/ui_stats.hpp"
#include "ui/ui_hotbar.hpp"
#include "ui/ui_root.hpp"
#include "ui/ui_inventory.hpp"
#include "entity/entity_player.hpp"
#include "gfx/sprite_resource.hpp"
#include "global.hpp"

static const auto spritesheet =
    SpritesheetResource("ui_stats", "ui/stats.png", { 8, 8 });

enum FillColor {
    FILL_COLOR_RED = 0,
    FILL_COLOR_ORANGE,
    FILL_COLOR_YELLOW,
    FILL_COLOR_BLUE,
    FILL_COLOR_GREEN,
    FILL_COLOR_COUNT
};

static const auto stat_internals =
    ValueManagedResource<std::array<TextureArea, 2>>(
        []() {
            std::array<TextureArea, 2> arr;
            for (usize i = 0; i < 2; i++) {
                const auto bounds =
                    AABB2u(UIStats::STAT_BAR_INTERNAL_SIZE)
                        .translate(
                            uvec2(
                                UIStats::STAT_BAR_INTERNAL_OFFSETS[i]));
                arr[i] =
                    spritesheet[AABB2u({ 0, 0 }, { 4, 0 })].subpixels(bounds);
            }
            return arr;
        });

static const auto stat_fills =
    ValueManagedResource<std::vector<TextureArea>>(
        []() {
            constexpr auto FILLS_OFFSET = uvec2(0, 1);
            std::vector<TextureArea> result;
            for (usize i = 0; i < FILL_COLOR_COUNT; i++) {
                result.push_back(
                    spritesheet[FILLS_OFFSET + uvec2(i, 0)]
                        .subpixels(UIStats::FILL_SIZE));
            }
            return result;
        });

UIStats &UIStats::configure(bool enabled) {
    Base::configure(SIZE, enabled);
    this->reposition();
    return *this;
}

void UIStats::reposition() {
    // position relative to hotbar
    const auto &hotbar = this->hud().hotbar;

    this->align(UIObject::ALIGN_CENTER_H);
    this->pos.y = hotbar->pos.y + hotbar->size.y;
}

void UIStats::render(SpriteBatch &batch, RenderState render_state) {
    const auto &sprites = spritesheet.get();
    const auto pos = this->pos_absolute();

    const auto base = sprites[AABB2u({ 0, 0 }, { 9, 0 })];
    batch.push(base, pos);

    const auto &player = this->hud().player();

    this->battery_lerped = player.battery().normalized();
    this->charge_lerped = player.charge.normalized();

    const auto
        pct_battery = this->battery_lerped.lerped(),
        pct_charge = this->charge_lerped.lerped();

    // find battery fill color
    const auto battery_fill_percentages =
        types::make_array(
            std::make_tuple(0.1f, FILL_COLOR_RED),
            std::make_tuple(0.2f, FILL_COLOR_ORANGE),
            std::make_tuple(0.3f, FILL_COLOR_YELLOW),
            std::make_tuple(1.0f, FILL_COLOR_GREEN));

    FillColor battery_fill_color = FILL_COLOR_GREEN;
    for (const auto &[pct, fill] : battery_fill_percentages) {
        if (pct_battery <= pct) {
            battery_fill_color = fill;
            break;
        }
    }

    struct Stat {
        TextureArea fill;
        f32 value;
        bool flash;
    };

    // abs distance from low power threshold, in range [1, LOW_POWER_THRESHOLD]
    const auto low_power_distance =
        *player.battery() < EntityPlayer::LOW_POWER_THRESHOLD ?
            usize(EntityPlayer::LOW_POWER_THRESHOLD - *player.battery())
            : 0;

    const auto
        flash_battery =
           low_power_distance > 0 ?
                ((global.time->ticks / (15 - low_power_distance)) % 2) == 0
                : false,
        flash_charge =
            player.charge_penalty_ticks > 0 ?
                ((global.time->ticks / 6) % 2 == 0)
                : (player.last_interaction
                   && (global.time->ticks
                        - player.last_interaction->ticks) < 4);

    const auto stats =
        types::make_array(
            Stat {
                stat_fills[battery_fill_color],
                pct_battery,
                flash_battery
            },
            Stat {
                stat_fills[FILL_COLOR_BLUE],
                pct_charge,
                flash_charge
            });

    for (usize i = 0; i < stats.size(); i++) {
        const auto &stat = stats[i];
        const auto stat_pos = pos + STAT_BAR_INTERNAL_OFFSETS[i];

        // draw a flashing version of the internal fill
        if (stat.flash) {
            batch.push(stat_internals[i], stat_pos, vec4(vec3(1.0f), 0.5f));
        }

        const auto color = stat.flash ? vec4(vec3(1.0f), 0.5f) : vec4(0.0f);
        const auto n =
            usize(math::floor(stat.value * STAT_BAR_INTERNAL_SIZE.x));

        for (usize j = 0; j < n; j++) {
            const auto k = j % (stat.fill.size().x - 1);
            const auto vertical_slice =
                stat.fill.subpixels(
                    AABB2u(uvec2(k, 0), uvec2(k, stat.fill.size().y)));
            batch.push(vertical_slice, stat_pos + ivec2(j, 0), color);
        }
    }
}
