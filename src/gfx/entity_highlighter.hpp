#pragma once

#include "entity/entity.hpp"
#include "gfx/program.hpp"
#include "util/time.hpp"
#include "global.hpp"

struct EntityHighlighter {
    static constexpr auto FADE_TICKS = 30;

    struct Entry {
        EntityId id;
        vec4 color;

        Entry() = default;
        Entry(
            EntityId id,
            const vec4 &color,
            usize tick_set = global.time->ticks)
            : id(id),
              color(color),
              tick_set(tick_set) {}

        inline bool done() {
            return (global.time->ticks - this->tick_set) >= FADE_TICKS;
        }

        inline vec4 get_faded_color() {
            const auto f =
                1.0f - (f32(global.time->ticks - this->tick_set) / FADE_TICKS);
            return vec4(
                this->color.rgb(),
                this->color.a * f);
        }

private:
        usize tick_set;
    };

    static inline auto NO_ENTRY = Entry(NO_ENTITY, vec4(0), 0);

    std::array<Entry, 3> entries = { NO_ENTRY, NO_ENTRY, NO_ENTRY };

    void set(const Entry &e) {
        for (auto &entry : this->entries) {
            if (entry.id == e.id) {
                entry = e;
                return;
            }
        }

        this->entries[this->index++] = e;
        this->index %= this->entries.size();
    }

    void tick() {
        for (auto &e : this->entries) {
            if (e.id != NO_ENTITY && e.done()) {
                e = NO_ENTRY;
            }
        }
    }

    inline void set_uniforms(
        const Program &program_edge,
        const Program &program_composite) {
        program_edge.try_set(
            "u_highlight_entities",
            vec4(
                this->entries[0].id,
                this->entries[1].id,
                this->entries[2].id,
                1.0));

        program_composite.try_set(
            "u_highlight_colors",
            types::make_array(
                this->entries[0].get_faded_color(),
                this->entries[1].get_faded_color(),
                this->entries[2].get_faded_color()),
            3);
    }

private:
    usize index;
};
