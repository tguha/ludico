#include "entity/entity_plant.hpp"
#include "entity/entity_mob.hpp"
#include "entity/entity_smoke_particle.hpp"
#include "entity/entity_smash_particle.hpp"
#include "entity/entity_item.hpp"
#include "model/model_entity.hpp"
#include "sound/sound_resource.hpp"
#include "util/rand.hpp"
#include "util/hash.hpp"
#include "util/time.hpp"
#include "serialize/serialize.hpp"
#include "global.hpp"

SERIALIZE_ENABLE()
DECL_SERIALIZER(EntityPlant)

static const auto sound_harvest = SoundResource("sound/harvest.wav");

void EntityPlant::on_grow(u8 old_stage) {
    // smoke particles with entity color on growth
    EntityParticle::spawn<EntitySmokeParticle>(
            *this->level,
            this->center,
            4, 7,
            this->colors());
}

InteractionAffordance EntityPlant::can_harvest(
    const Entity &other,
    ItemRef item) const {
    return InteractionAffordance::YES;
}

InteractionResult EntityPlant::harvest(
    const Entity &other,
    ItemRef item) {
    sound_harvest->play();
    this->smash(other);
    return InteractionResult::SUCCESS;
}

void EntityPlant::smash(EntityRef other) {
    const auto stage_drops = this->stage_drops();
    EntityItem::drop(
        *this->level,
        this->center,
        stage_drops.contains(this->stage) ?
            stage_drops.at(this->stage)
            : DropTable::empty());
    EntityParticle::spawn<EntitySmashParticle>(
        *this->level,
        this->center,
        4, 8,
        this->colors());
    this->destroy();
}

bool EntityPlant::does_stop(const Entity &other) const {
    return !EntityRef(other).as<EntityMob>().present();
}

InteractionAffordance EntityPlant::can_interact_with_this(
    InteractionKind kind,
    const Entity &other,
    ItemRef item) const {
    if (kind != InteractionKind::PRIMARY) {
        return InteractionAffordance::NONE;
    }

    return this->can_harvest(other, item);
}

InteractionResult EntityPlant::interacted(
    InteractionKind kind,
    Entity &other,
    ItemRef item) {
    const auto result = this->harvest(other, item);

    if (result == InteractionResult::SUCCESS) {
        this->destroy();
    }

    return result;
}

void EntityPlant::tick() {
    if (!this->grounded()) {
        this->smash(EntityRef::none());
        return;
    }

    if (this->stage >= this->num_stages() - 1) {
        return;
    }

    const auto ticks_to_next_stage =
        usize(
            TICKS_PER_GROWTH_STAGE
                * Rand(hash(this->id, this->stage)).next_normal(
                    this->growth_speed(),
                    math::pow(this->growth_variance(), 2.0f)));

    if (this->last_stage_ticks + ticks_to_next_stage <= global.time->ticks) {
        this->last_stage_ticks = global.time->ticks;
        this->stage++;
        this->on_grow(this->stage - 1);
    }
}
