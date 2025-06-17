#include "item/item_container.hpp"
#include "item/item.hpp"
#include "level/level.hpp"
#include "serialize/serialize.hpp"

SERIALIZE_ENABLE()
DECL_SERIALIZER(ItemContainer::Slot)
DECL_SERIALIZER(ItemContainer)
DECL_SERIALIZER(ItemContainer)
DECL_PRIMITIVE_SERIALIZER(std::vector<ItemContainer::Slot>)

static void init(ItemContainer &ic, usize capacity) {
    ic.tracker = ItemContainer::Tracker(&ic, &ItemContainer::tracker);
    for (usize i = 0; i < capacity; i++) {
        ic.slots.emplace_back(ItemContainer::Slot(ic, i));
    }
}

ItemContainer::ItemContainer(
    Level &level,
    usize capacity,
    EntityRef owner)
    : id(level.next_item_container_id()),
      owner(owner) {
    init(*this, capacity);
}

ItemContainer ItemContainer::make_basic(usize capacity) {
    ItemContainer ic;
    init(ic, capacity);
    return ic;
}

std::vector<ItemStack*> ItemContainer::stacks_of(const Item &item) {
    std::vector<ItemStack*> res;

    for (auto &slot : *this) {
        if (!slot.hidden() && !slot.empty() && slot->item().id == item.id) {
            res.push_back(&*slot);
        }
    }

    return res;
}

ItemStack *ItemContainer::first_nonfull_stack_of(const Item &item) {
    auto stacks = this->stacks_of(item);

    auto it = std::find_if(
        stacks.begin(), stacks.end(),
        [&](ItemStack *&stack) {
            return stack->size < stack->item().stack_size();
        });

    return it != stacks.end() ? *it : nullptr;
}

ItemStack *ItemContainer::find_stack_of(
    const Item &item) {
    auto it = std::find_if(
        this->begin(), this->end(),
        [&](const auto &slot) {
            return !slot.hidden()
                && !slot.empty()
                && slot->item().id == item.id;
        });

    return it != this->end() ?  &**it : nullptr;
}

ItemStack *ItemContainer::find_stack_of(ItemStackId id) {
    auto it = std::find_if(
        this->begin(), this->end(),
        [&](const auto &slot) {
            return !slot.hidden()
                && !slot.empty()
                && slot->id == id;
        });

    return it != this->end() ?  &**it : nullptr;
}

usize ItemContainer::can_add(const ItemStack &stack) const {
    const auto slot = this->open_slot(&stack.item());

    if (slot != -1) {
        return stack.size;
    }

    // cannot add if no open slots and not stackable
    if (stack.item().stack_size() == 1) {
        return 0;
    }

    // check if we can stack
    const auto existing =
        const_cast<ItemContainer*>(this)->stacks_of(stack.item());

    usize available = 0;
    for (const auto *s : existing) {
        available += s->item().stack_size() - s->size;
    }

    return available >= stack.size;
}

std::tuple<ItemStack*, std::optional<ItemStack>> ItemContainer::try_add(
    ItemStack &&stack,
    usize index) {
    // item stack with ID cannot already exist
    ASSERT(
        !this->find_stack_of(stack.id),
        "stack with id {} ({}) already exists in container at {}",
        stack.id,
        stack.to_string(),
        fmt::ptr(this));

    if (index != NO_INDEX) {
        auto &slot = (*this)[index];

        if (slot.empty()) {
            slot = std::move(stack);
            return { &*slot, std::nullopt };
        } else if (types::are_same(slot->item(), stack.item())) {
            const auto
                total = slot->size + stack.size,
                remainder =
                    total > slot->item().stack_size() ?
                        total - slot->item().stack_size()
                        : 0,
                added = stack.size - remainder;

            slot->size += added;
            stack.size -= added;
            return {
                &*slot,
                stack.size == 0 ?
                    std::nullopt
                    : std::make_optional(std::move(stack))
            };
        } else {
            return { nullptr, std::nullopt };
        }
    }

    if (stack.item().stack_size() == 1) {
        const auto slot = this->open_slot(&stack.item());
        if (slot == -1) {
            return std::make_tuple(nullptr, std::move(stack));
        }

        (*this)[slot] = std::move(stack);
        return std::make_tuple(&*(*this)[slot], std::nullopt);
    }

    // stack in existing stacks
    usize n = stack.size;
    ItemStack *existing = nullptr;
    while (n != 0 && (existing = this->first_nonfull_stack_of(stack.item()))) {
        const auto
            total = existing->size + stack.size,
            remainder =
                total > existing->item().stack_size() ?
                    total - existing->item().stack_size()
                    : 0,
            added = n - remainder;

        existing->size += added;
        stack.size -= added;
        n = remainder;
    }

    // add stack if there are remaining items
    if (stack.size != 0) {
        const auto slot = this->open_slot(&stack.item());

        if (slot == -1) {
            return std::make_tuple(existing, std::move(stack));
        }

        (*this)[slot] = std::move(stack);
        existing = &*(*this)[slot];
    }

    return std::make_tuple(existing, std::nullopt);
}

ItemStack ItemContainer::remove(const ItemStack &stack, usize n) {
    ASSERT(n <= stack.size);
    auto it = std::find_if(
        this->slots.begin(), this->slots.end(),
        [&](const auto &slot) {
            return !slot.empty()
                && slot->id == stack.id;
        });
    ASSERT(it != this->slots.end());

    ItemStack &s = *this->slots[it - this->slots.begin()];
    ASSERT(&s == &stack);

    ItemStack result;

    if (n == stack.size) {
        // remove complete stack
        result = std::move(s);
        (*this)[it - this->slots.begin()] = std::nullopt;
    } else {
        result = stack.clone();
        result.size = n;
        s.size -= n;
    }

    return result;
}
