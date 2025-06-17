#pragma once

#include "entity/util.hpp"
#include "util/util.hpp"
#include "serialize/annotations.hpp"

struct Level;
struct SerializationContextLevel;

namespace detail {
    Entity *entity_from_id(const Level *level, EntityId id);

    Level *get_level(const Entity *e);

    EntityId get_id(const Entity *e);

    std::string name(const Entity *e);
}

// TODO: cache ptr if tick is the same?
struct BaseEntityRef {
    [[SERIALIZE_BY_CTX(SERIALIZE_IGNORE, SerializationContextLevel::get_level)]]
    Level *level = nullptr;

    EntityId id = NO_ENTITY;

    BaseEntityRef() = default;
    BaseEntityRef(Level *level, EntityId id)
        : level(level),
          id(id) {}

    void on_after_deserialize(SerializationContextLevel &ctx);
};

template <typename E>
struct [[SERIALIZE_AS(BaseEntityRef)]] IEntityRef : BaseEntityRef {
    IEntityRef() = default;
    IEntityRef(Level &level, EntityId id)
        : BaseEntityRef(&level, id) {}

    // conversion from Entity pointer
    IEntityRef(const E *entity)
        : BaseEntityRef(
            entity ? detail::get_level(entity) : nullptr,
            entity ? detail::get_id(entity) : NO_ENTITY) {}

    // conversion from Entity reference
    IEntityRef(const E &entity)
        : IEntityRef(&entity) {}

    // convert from E -> base class F (upcast, always valid)
    template <typename F>
        requires std::is_base_of_v<F, E>
    inline operator IEntityRef<F>() const {
        E *ptr = this->ptr();
        return ptr ?
            IEntityRef<F>(*this->level, this->id)
            : IEntityRef<F>::none();
    }

    // convert from E -> child class (downcast, runtime check)
    // returns IEntityRef<F> if conversion was invalid
    template <typename F>
        requires std::is_base_of_v<E, F>
    inline operator IEntityRef<F>() const {
        // check type, return "none" if could not be cast
        E *ptr = this->ptr();
        return ptr && dynamic_cast<F*>(ptr) ?
            IEntityRef<F>(*this->level, this->id)
            : IEntityRef<F>::none();
    }

    inline bool operator==(const IEntityRef &other) const {
        return this->level == other.level
            && this->id == other.id;
    }

    // true if entity can be found
    bool present() const {
        return this->level && detail::entity_from_id(this->level, this->id);
    }

    // true if present()
    inline operator bool() const {
        return this->present();
    }

    // gets entity reference from id
    // crashes if not present
    E &get() const {
        return *this->ptr();
    }

    inline E &operator*() const {
        return this->get();
    }

    inline operator E&() const {
        return **this;
    }

    inline operator E*() const {
        return this->ptr();
    }

    inline operator EntityId() const {
        return this->id;
    }

    // creates optional entity pointer from this reference
    std::optional<E*> opt() const {
        E *ptr = this->ptr();
        return ptr ? std::make_optional(ptr) : std::nullopt;
    }

    inline operator std::optional<E*>() const {
        return this->opt();
    }

    inline E *ptr() const {
        if (!this->level || this->id == NO_ENTITY) {
            return nullptr;
        }

        Entity *e = detail::entity_from_id(this->level, this->id);
        if constexpr (std::is_same_v<E, Entity>) {
            return e;
        } else {
            return e ? dynamic_cast<E*>(e) : nullptr;
        }
    }

    inline E *operator->() const {
        return &**this;
    }

    std::string to_string() const {
        return fmt::format(
            "EntityRef({},{},{})",
            fmt::ptr(this->level),
            this->id,
            this->present() ? detail::name(this->ptr()) : "(NOT PRESENT)");
    }

    // cast an entity ref to entity type E, returns IEntityRef::none() if cast
    // fails
    template <typename F>
    IEntityRef<F> as() const {
        if (const auto *f = dynamic_cast<F*>(this->ptr())) {
            return IEntityRef<F>(f);
        }

        return IEntityRef<F>::none();
    }

    static inline IEntityRef none() {
        return IEntityRef();
    }
};

// base entity ref to any entity type
using EntityRef = IEntityRef<Entity>;

// ensure IEntityRef types are always serializable
SERIALIZABLE_TYPE_REGEX("IEntityRef.*")
