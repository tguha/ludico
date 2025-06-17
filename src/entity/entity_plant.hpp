#pragma once

#include "entity/entity.hpp"
#include "entity/entity_modeled.hpp"
#include "entity/entity_colored.hpp"
#include "item/drop_table.hpp"
#include "util/time.hpp"
#include "locale.hpp"

// TODO: remove
#include "level/light.hpp"

struct EntityPlant
    : virtual public Entity,
      virtual public EntityModeled,
      virtual public EntityColored {
    using Ref = IEntityRef<EntityPlant>;
    using GrowthStage = u8;

    // 15 minutes/growth stage
    static constexpr auto TICKS_PER_GROWTH_STAGE =
        TICKS_PER_SECOND * (60 * 15);

    // drops list for growth stages
    using StageDrops = std::unordered_map<GrowthStage, DropTable>;

    // current growth stage
    GrowthStage stage = 0;

    // time since last growth stage reached
    usize last_stage_ticks;

    // growth speed, based on TICKS_PER_GROWTH_STAGE
    virtual f32 growth_speed() const { return 1.0f; };

    // variance in growth time normal distribution
    virtual f32 growth_variance() const { return 0.001f; };

    // number of growth stages
    virtual GrowthStage num_stages() const = 0;

    // called whenever plant grows
    virtual void on_grow(GrowthStage old_stage);

    // drops on harvest
    virtual StageDrops stage_drops() const = 0;

    // current drops
    virtual DropTable drops() const {
        const auto stages = this->stage_drops();
        return stages.contains(this->stage) ?
            stages.at(this->stage)
            : DropTable::empty();
    }

    // returns charge value if other can harvest this plant
    virtual InteractionAffordance can_harvest(
        const Entity &other,
        ItemRef item) const;

    virtual InteractionResult harvest(
        const Entity &other,
        ItemRef item);

    virtual void smash(EntityRef other);

    bool does_stop(const Entity &other) const override;

    InteractionAffordance can_interact_with_this(
        InteractionKind kind,
        const Entity &other,
        ItemRef item) const override;

    InteractionResult interacted(
        InteractionKind kind,
        Entity &other,
        ItemRef item) override;

    bool highlight() const override { return true; }

    void tick() override;
};
