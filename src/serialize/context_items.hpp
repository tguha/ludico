#pragma once

#include "item/item_container.hpp"
#include "serialize/context.hpp"

struct SerializationContextItems
    : virtual public SerializationContext {
    // item containers which have been deserialized
    [[SERIALIZE_IGNORE]]
    std::unordered_map<ItemContainerId, ItemContainer*> containers;

    void notify_deserialize(const arc::any &ptr, arc::type_id id) override {
        if (id == arc::type_id::from<ItemContainer>()) {
            ItemContainer *ic = ptr.as<ItemContainer*>();
            this->containers.emplace(ic->id, ic);
        }
    }

    ItemContainer *lookup_id(ItemContainerId id) const {
        auto it = this->containers.find(id);
        ASSERT(
            it != this->containers.end(),
            "no item container with id {}", id);
        return it->second;
    }
};
