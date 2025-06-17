#pragma once

#include "util/util.hpp"
#include "item/item_stack.hpp"

struct ItemMetadata {
    virtual ~ItemMetadata() = default;

    virtual std::string to_string() const {
        return "ItemMetadata()";
    }
};

template <typename T>
concept IsItemMetadata =
    (std::is_base_of_v<ItemMetadata, T>
        && std::is_move_constructible_v<T>);

template <typename T, typename M>
struct ItemMetadataType {
    using Metadata = M;

    static inline M &get_metadata(const ItemStack &stack) {
        ASSERT(
            types::instance_of<T>(stack.item()),
            "stack item not instance of {}",
            typeid(T).name());
        return stack.metadata<M>();
    }

    virtual M &metadata(const ItemStack &stack) const {
        return ItemMetadataType<T, M>::get_metadata(stack);
    }
};
