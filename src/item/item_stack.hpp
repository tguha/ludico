#pragma once

#include "util/moveable.hpp"
#include "util/assert.hpp"
#include "util/alloc_ptr.hpp"
#include "serialize/annotations.hpp"
#include "item/util.hpp"

struct ItemMetadata;
struct Level;
struct Item;

// "stack" of one or more items, uniquely identified by its ID
// non-copyable
struct ItemStack {
    ItemStackId id;
    usize size = 0;

    ItemStack() = default;
    ItemStack(const ItemStack&) = delete;
    ItemStack(ItemStack&&) = default;
    ItemStack &operator=(const ItemStack&) = delete;
    ItemStack &operator=(ItemStack&&) = default;

    ItemStack(Level &level, const Item &item, usize size = 1);

    inline bool has_metadata() const {
        return this->_metadata != nullptr;
    }

    template <typename T>
    inline T &metadata() const {
        T *t = dynamic_cast<T*>(this->_metadata.get());
        ASSERT(t, "metadata is not of type {}", typeid(T).name());
        return *t;
    }

    template <typename T>
    inline T &add_metadata(T &&t, Allocator &allocator) {
        return this->add_metadata<T>(
            make_alloc_ptr<T>(
                allocator,
                std::forward<T>(t)));
    }

    template <typename T>
    inline T &add_metadata(alloc_ptr<T> &&t) {
        this->_metadata = std::move(t);
        return *this->_metadata;
    }

    inline Level &level() const {
        return *this->_level;
    }

    inline const Item &item() const {
        return *this->_item;
    }

    // creates a unique clone of this item stack
    inline ItemStack clone() const {
        return ItemStack(
            *this->_level,
            *this->_item,
            this->size);
    }

    std::string to_string() const;

private:
    [[SERIALIZE_BY_CTX(SERIALIZE_IGNORE, SerializationContextLevel::get_level)]]
    Level *_level;

    [[SERIALIZE_CUSTOM(SerializerItemPtr)]]
    const Item *_item;

    alloc_ptr<ItemMetadata> _metadata;
};
