#pragma once

#include "entity/entity.hpp"
#include "entity/interaction.hpp"
#include "entity/entity_modeled.hpp"
#include "item/item_container.hpp"
#include "util/time.hpp"
#include "util/side.hpp"
#include "serialize/annotations.hpp"

struct ModelEntity;

struct EntityMob :
    virtual public Entity,
    virtual public EntityModeled {
    using Base = Entity;
    using Ref = IEntityRef<EntityMob>;

    EntityMob() = default;

    static constexpr auto SPEED_BY_TICKS = 0.60f / f32(TICKS_PER_SECOND);

    // facing direction
    LerpDir facing = LerpDir(20);

    // current interaction this entity could perform if initiated (the
    // interaction which exists before a click is started)
    std::optional<Interaction> potential_interaction;

    // current interaction this entity is charging for
    std::optional<Interaction> current_interaction;

    // last interaction performed by this entity
    std::optional<Interaction> last_interaction;

    virtual void set_held_slot(ItemContainer::Slot &slot) {
        ASSERT(this->inventory());
        ASSERT(&slot.parent() == this->inventory());
        this->held_slot = slot;
    }

    // returns currently held item stack, ItemRef::none() if not present
    virtual ItemRef held() const {
        return this->held_slot ?
            ItemRef::from_slot(this->held_slot)
            : ItemRef::none();
    }

    // offset of held item from base position
    virtual vec3 hold_offset() const { return vec3(0); }

    // movement speed
    virtual f32 speed() const {
        return 1.0f;
    }

    // reach (in tiles)
    virtual f32 reach() const {
        return 4.0f;
    }

    bool can_reach(const vec3 &target) const;

    bool can_reach(const ivec3 &target) const;

    bool does_collide(const Entity &other) const override { return true; }

    bool does_stop(const Entity &other) const override { return true; }

    void tick() override;

    LightArray lights() const override;

    virtual Side::Enum side_hold() const {
        return Side::LEFT;
    }

private:
    // reference to the item slot of the currently held item
    ItemContainer::SlotRef held_slot;
};
