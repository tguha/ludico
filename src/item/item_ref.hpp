#pragma once

#include "util/util.hpp"
#include "util/types.hpp"
#include "item/item_stack.hpp"
#include "item/item_container.hpp"
#include "serialize/annotations.hpp"

struct SerializationContextItems;

// reference to an item in a container which can be invalidated
struct ItemRef {
    ItemRef() = default;

    ItemRef(ItemContainer &container, ItemStackId stack_id)
        : _parent(&container),
          _id(stack_id) {}

    ItemRef(ItemContainer &container, const ItemStack &stack)
        : ItemRef(container, stack.id) {}

    explicit ItemRef(const ItemContainer::Slot &slot) {
        if (!slot.empty()) {
            this->_parent = &slot.parent();
            this->_id = slot->id;
        }
    }

    // get container
    inline ItemContainer &parent() const {
        return *this->_parent;
    }

    // get id of underlying stack
    inline ItemStackId id() const {
        return this->_id;
    }

    inline auto operator==(const ItemRef &other) const {
        return this->_parent == other._parent
            && this->_id == other._id;
    }

    // access underlying item stack
    auto operator->() const {
        return &**this;
    }

    // returns true iff: this is not ::none() && item is present in container
    operator bool() const;

    // dereference to item stack
    // WARNING: crashes if not present
    ItemStack &operator*() const;

    // value representing an invalid item ref
    static inline ItemRef none() {
        return ItemRef();
    }

    std::string to_string() const {
        return fmt::format(
            "ItemRef({},{})",
            fmt::ptr(this->_parent),
            this->_id);
    }

    // TODO: necessary? just use the ctor
    static auto from_slot(const ItemContainer::Slot &slot) {
        return static_cast<ItemRef>(slot);
    }

private:
    [[SERIALIZE_BY_ID(SerializationContextItems, ItemContainer::id)]]
    ItemContainer *_parent = nullptr;

    ItemStackId _id = 0;
};
