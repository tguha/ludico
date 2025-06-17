#pragma once

#include "item/item_stack.hpp"
#include "item/util.hpp"
#include "entity/entity_ref.hpp"
#include "entity/util.hpp"
#include "util/int_range.hpp"
#include "util/tracker.hpp"

struct Item;

// item container
struct ItemContainer {
    using Tracker = Tracker<ItemContainer>;

    static constexpr auto NO_INDEX = std::numeric_limits<usize>::max();

    // represents a single "slot" of an item container
    // behaves similarly to std::optional<ItemStack> except no implicit
    // conversion to bool (use .empty()).
    // safe to store references and pointers as long as lifetime does not exceed
    // that of parent.
    struct Slot {
        Slot() = default;

        Slot(ItemContainer &parent, usize index)
            : _ref(parent.tracker.ref()),
              _parent(&parent),
              _index(index) {}

        Slot(Slot&&) = default;
        Slot(const Slot&) = delete;
        Slot &operator=(Slot&&) = default;
        Slot &operator=(const Slot&) = delete;

        void on_before_serialize(SerializationContext&) {
            this->_parent = this->_ref ? this->_ref.get() : nullptr;
        }

        void on_resolve(SerializationContext&) {
            if (this->_parent) {
                this->_ref = this->_parent->tracker.ref();
            }
        }

        auto &operator*() const { return *this->_value; }
        auto &operator*() { return *this->_value; }

        auto *operator->() const { return this->_value.operator->(); }
        auto *operator->() { return this->_value.operator->(); }

        bool hidden() const {
            return this->_ref && this->_ref->hidden(this->index());
        }

        bool empty() const {
            return !this->_ref || !this->_value.has_value();
        }

        template <typename T>
        auto &operator=(T &&t) {
            if constexpr (std::is_same_v<std::decay_t<T>, Slot>) {
                this->_ref = std::move(t._ref);
                this->_parent = t._parent;
                this->_index = t._index;
                this->_value = std::move(t._value);
            } else {
                this->_value.operator=(std::forward<T>(t));
            }

            return *this;
        }

        ItemContainer &parent() const {
            return *_ref;
        }

        usize index() const {
            return this->_index;
        }

        std::string to_string() const {
            return fmt::format(
                "Slot({},{},{})",
                this->_ref ? fmt::ptr(this->_ref.get()) : "nullptr",
                this->_index,
                this->_value);
        }

    private:
        [[SERIALIZE_IGNORE]]
        Tracker::Ref _ref;

        [[SERIALIZE_BY_ID(SerializationContextItems, ItemContainer::id)]]
        ItemContainer *_parent;

        usize _index;
        std::optional<ItemStack> _value = std::nullopt;
    };

    // reference to a slot in an item container
    struct SlotRef {
        SlotRef() = default;

        SlotRef(ItemContainer &parent, usize index)
            : _ref(parent.tracker.ref()),
              _parent(&parent),
              _index(index) {}

        SlotRef(Slot *slot)
            : _ref(slot ? slot->parent().tracker.ref() : Tracker::Ref()),
              _parent(slot ? &slot->parent() : nullptr),
              _index(slot ? slot->index() : -1) {}

        SlotRef(Slot &slot)
            : SlotRef(&slot) {}

        void on_before_serialize(SerializationContext&) {
            this->_parent = this->_ref ? this->_ref.get() : nullptr;
        }

        void on_resolve(SerializationContext&) {
            if (this->_parent) {
                this->_ref = this->_parent->tracker.ref();
            }
        }

        inline operator Slot&() const {
            return **this;
        }

        inline Slot &slot() const {
            return **this;
        }

        inline operator bool() const {
            return (*this) != SlotRef::none();
        }

        inline bool operator==(const SlotRef &rhs) const {
            return this->_ref == rhs._ref
                && this->_index == rhs._index;
        }

        // get contents underlying slot
        inline ItemStack &stack() const {
            return *(*this->_ref)[this->_index];
        }

        // access underlying slot
        inline Slot &operator*() const {
            return (*this->_ref)[this->_index];
        }

        // access underlying slot
        inline Slot *operator->() {
            return &**this;
        }

        static inline SlotRef none() {
            return SlotRef();
        }

        std::string to_string() const {
            return fmt::format(
                "SlotRef({})",
                *this ? this->slot().to_string() : "none");
        }

    private:
        [[SERIALIZE_IGNORE]]
        Tracker::Ref _ref;

        [[SERIALIZE_BY_ID(SerializationContextItems, ItemContainer::id)]]
        ItemContainer *_parent = nullptr;

        usize _index = -1;
    };

    // tracker for this container
    [[SERIALIZE_IGNORE]]
    Tracker tracker;

    // unique ID for this container
    ItemContainerId id;

    // slot data
    mutable std::vector<Slot> slots;

    // owner of this container, if applicable
    EntityRef owner;

    ItemContainer() = default;

    ItemContainer(
        Level &level,
        usize capacity,
        EntityRef owner = EntityRef::none());

    ItemContainer(ItemContainer &&other) = default;
    ItemContainer &operator=(ItemContainer &&other) = default;

    ItemContainer(const ItemContainer&) = delete;
    ItemContainer &operator=(const ItemContainer&) = delete;

    virtual ~ItemContainer() = default;

    void on_after_deserialize(SerializationContext&) {
        this->tracker = Tracker(this, &ItemContainer::tracker);
    }

    // makes a "basic" item container which cannot be serialized and has no
    // owner
    static ItemContainer make_basic(usize capacity);

    std::vector<Slot*> range_slots(const IntRange<usize> &range) const {
        ASSERT(
            range.min >= 0 && range.max < this->size(),
            "range {} not valid for size {}",
            range,
            this->size());
        std::vector<Slot*> result;
        result.reserve(range.count());
        for (usize i = range.min; i <= range.max; i++) {
            result.push_back(&this->slots[i]);
        }
        return result;
    }

    // override to disallow items by slot
    virtual bool allowed(usize i, const Item &item) const {
        return true;
    }

    // returns true if specified slot is "hidden" for special use
    // if hidden, a slot will still be a part of the inventory but not found
    // through searching functions, etc.
    virtual bool hidden(usize i) const {
        return false;
    }

    auto begin() const {
        return slots.begin();
    }

    auto end() const {
        return slots.end();
    }

    auto begin() {
        return slots.begin();
    }

    auto end() {
        return slots.end();
    }

    inline usize size() const {
        return this->slots.size();
    }

    inline Slot &operator[](usize i) {
        return this->slots[i];
    }

    inline const Slot &operator[](usize i) const {
        return this->slots.at(i);
    }

    // finds an open slot in this inventory, -1 if there is no such slot
    inline isize open_slot(const Item *for_item = nullptr) const {
        for (usize i = 0; i < this->size(); i++) {
            if (this->hidden(i)
                || (for_item && !this->allowed(i, *for_item))) {
                continue;
            }

            if ((*this)[i].empty()) {
                return i;
            }
        }

        return -1;
    }

    // returns -1 if item not found
    // NOTE: also searches hidden slots as the search is id-based
    inline isize index_of(ItemStackId id) const {
        auto it = std::find_if(
            this->begin(), this->end(),
            [&](const Slot &s) {
                return !s.empty() && s->id == id;
            });
        return it == this->end() ? -1 : (it - this->begin());
    }

    // returns -1 if item not found
    inline isize index_of(const ItemStack &stack) const {
        return this->index_of(stack.id);
    }

    // returns true if container contains item with id
    inline bool contains(ItemStackId id) const {
        return this->index_of(id) != -1;
    }

    // returns true if container contains stack
    inline bool contains(const ItemStack &stack) const {
        return this->index_of(stack) != -1;
    }

    // returns all stacks of the specified item
    std::vector<ItemStack*> stacks_of(const Item &item);

    // tries to find the first empty stack of the specified item
    ItemStack *first_nonfull_stack_of(const Item &item);

    // finds stack of item type
    ItemStack *find_stack_of(const Item &item);

    // finds stack of item stack
    ItemStack *find_stack_of(const ItemStack &stack) {
        return this->find_stack_of(stack.id);
    }

    // finds stack of item id
    ItemStack *find_stack_of(ItemStackId id);

    // returns number of items out of stack which can be added to this container
    usize can_add(const ItemStack &stack) const;

    // try to add the item stack into this inventory, returns
    // (pointer to added items, remainder of items which could not be added)
    //
    // use "index" to indicate which slot to try to add to - will add anywhere
    // (prioritizing lower slots indices) if index is NO_INDEX
    std::tuple<ItemStack*, std::optional<ItemStack>> try_add(
        ItemStack &&stack,
        usize index = NO_INDEX);

    // n of the specified item type from the inventory
    // NOTE: assumes existence of items
    inline ItemStack remove(const Item &item, usize n) {
        return this->remove(*this->find_stack_of(item), n);
    }

    // remove all of the specified item stack
    inline ItemStack remove(const ItemStack &stack) {
        return this->remove(stack, stack.size);
    }

    // remove n items out of the specified item stack
    ItemStack remove(const ItemStack &stack, usize n);
};
