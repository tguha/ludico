#pragma once

#include "level/level.hpp"
#include "serialize/annotations.hpp"
#include "serialize/context_items.hpp"

#include <archimedes.hpp>

struct EntityPlayer;

struct SerializationContextLevel
    : virtual public SerializationContextItems {
    using Base = SerializationContextItems;

    [[SERIALIZE_IGNORE]] Allocator *base_allocator = nullptr;
    [[SERIALIZE_IGNORE]] Level *level = nullptr;
    [[SERIALIZE_IGNORE]] EntityPlayer *player = nullptr;

    SerializationContextLevel() = default;

    void notify_alloc(const arc::any &ptr, arc::type_id id) override {
        Base::notify_alloc(ptr, id);

        if (id == arc::type_id::from<Level>()) {
            this->level = ptr.as<Level*>();
        } else if (id == arc::type_id::from<EntityPlayer>()) {
            this->player = ptr.as<EntityPlayer*>();
        }
    }

    Allocator &allocator(arc::type_id id) override {
        if (!this->level) {
            // pre-level, allocate on base_allocator
            return *this->base_allocator;
        }

        return this->level->poly_allocator.allocator(id);
    }

    Level *get_level() const {
        return this->level;
    }
};
