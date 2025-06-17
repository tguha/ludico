#include "item/drop_table.hpp"
#include "item/item_stack.hpp"
#include "item/item.hpp"
#include "util/hash.hpp"
#include "util/rand.hpp"
#include "util/time.hpp"
#include "global.hpp"

std::vector<ItemStack> DropTable::get(Level &level) const {
    auto rand = Rand(hash(global.time->ticks));

    std::vector<ItemStack> res;
    for (usize i = 0; i < this->n_drops; i++) {
        const auto &d = this->drops[i];

        if (rand.next(0.0f, 1.0f) > d.chance) {
            continue;
        }

        const auto count = rand.next(d.min_count, d.max_count);
        if (count == 0) {
            continue;
        }

        ItemStack stack(level, *d.item);

        if (stack.item().stack_size() > 1) {
            stack.size = count;
            res.push_back(std::move(stack));
        } else {
            usize n = 1;
            while (n < count) {
                res.push_back(stack.clone());
            }
            res.push_back(std::move(stack));
        }
    }

    return res;
}
