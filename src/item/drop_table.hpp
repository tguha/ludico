#pragma once

#include "util/util.hpp"
#include "util/types.hpp"
#include "util/assert.hpp"

struct Level;
struct ItemStack;
struct Item;

struct DropTable {
    struct Drop {
        const Item *item = nullptr;
        f32 chance; // [0.0f, 1.0f]
        usize min_count = 1;
        usize max_count = 1;
    };

    static constexpr auto MAX_DROPS = 16;

    usize n_drops = 0;
    std::array<Drop, MAX_DROPS> drops;

    DropTable() = default;
    DropTable(Drop drop)
        : DropTable({ drop }) {}
    DropTable(const Item &item, f32 chance = 1.0f)
        : DropTable(Drop { &item, chance, 1, 1 }) {}

    DropTable(std::initializer_list<Drop> &&drops) {
        ASSERT(drops.size() < MAX_DROPS);

        for (const auto &d : drops) {
            this->drops[this->n_drops++] = d;
        }
    }

    std::vector<ItemStack> get(Level &level) const;

    static inline DropTable empty() {
        return DropTable();
    }
};
