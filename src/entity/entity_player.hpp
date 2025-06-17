#pragma once

#include "entity/entity_mob.hpp"
#include "entity/stat.hpp"
#include "model/model_player.hpp"
#include "util/int_range.hpp"
#include "util/map.hpp"
#include "serialize/annotations.hpp"
#include "essence/essence.hpp"
#include "locale.hpp"

struct EntityPlayerInventory
    : public ItemContainer {
    using Base = ItemContainer;

    static constexpr IntRange<usize>
        RANGE_HOTBAR = { 0, 4 },
        RANGE_BASE = { 5, 20 },
        RANGE_BATTERY_GRID = { 21, 24 },
        RANGE_GENERATOR = { 25, 25 },
        RANGE_LOGIC_BOARD = { 26, 26 },
        RANGE_TOOLS = { 27, 28 },
        RANGE_NON_HIDDEN =
            IntRange<usize>::merge(
                RANGE_BASE,
                RANGE_HOTBAR),
        RANGE_HIDDEN =
            IntRange<usize>::merge(
                RANGE_BATTERY_GRID,
                RANGE_GENERATOR,
                RANGE_LOGIC_BOARD,
                RANGE_TOOLS);

    static constexpr auto
        SIZE = RANGE_TOOLS.max + 1,
        SIZE_NON_HIDDEN = RANGE_BASE.max + 1;

    EntityPlayerInventory() = default;
    EntityPlayerInventory(const EntityPlayer &player);

    bool hidden(usize i) const override {
        return RANGE_HIDDEN.contains(i);
    }

    bool allowed(usize i, const Item &item) const override;

    inline std::vector<Slot*> hotbar() const {
        return this->range_slots(RANGE_HOTBAR);
    }

    inline std::vector<Slot*> batteries() const {
        return this->range_slots(RANGE_BATTERY_GRID);
    }

    inline Slot &generator() const {
        return this->slots[RANGE_GENERATOR[0]];
    }

    inline Slot &logic_board() const {
        return this->slots[RANGE_LOGIC_BOARD[0]];
    }

    inline std::vector<Slot*> tools() const {
        return this->range_slots(RANGE_TOOLS);
    }

    inline Slot &hammer() const {
        return *this->tools()[0];
    }

    inline Slot &claw() const {
        return *this->tools()[1];
    }
};

struct EntityPlayer : public EntityMob {
    using Base = EntityMob;
    using Ref = IEntityRef<EntityPlayer>;

    enum class BlinkerState {
        DEFAULT = 0,
        LOW_POWER,
        FLIGHT,
        COUNT
    };

    static constexpr auto
        THRUST_POWER = -GRAVITY * 0.6f,
        THRUST_DRAG = vec3(0.85f),
        THRUST_MAX = -GRAVITY * 1.8f;

    // ticks before recharge begins after action
    static constexpr auto RECHARGE_WAIT_TICKS = 80;

    // ticks where recharge is in "penalty" state after zeroing
    static constexpr auto CHARGE_ZERO_PENALTY_TICKS = 120;

    // charge for interactions per-tick
    static constexpr auto INTERACTION_CHARGE_RATE = 0.05f;

    // recharge rate per-tick
    static constexpr auto CHARGE_RECHARGE_RATE = 0.05f;

    // charge cost of flight per-tick
    static constexpr auto FLIGHT_CHARGE_COST = 0.025f;

    // threshold for low power mode
    static constexpr auto LOW_POWER_THRESHOLD = 10.0f;

    // model animation state
    [[SERIALIZE_IGNORE]]
    ModelPlayer::AnimationState animation_state;

    // charge limit
    static constexpr auto CHARGE_MAX = 5.0f;

    // if > 0, charge is in zero-penalty state (cannot recharge)
    usize charge_penalty_ticks = 0;

    // current charge
    Stat<f32> charge = Stat<f32>(CHARGE_MAX, 0.0f, CHARGE_MAX);

    // current charge dedicated to potential interaction
    f32 interaction_charge = 0.0f;

    // blinker state, see enum
    BlinkerState blinker_state = BlinkerState::DEFAULT;

    // ticks when charge last changed
    u64 last_charge_change_ticks = std::numeric_limits<u64>::max();

    // true if there is a footstep this tick
    bool footstep = false;

    // acceleration from thrust added to velocity vector per tick
    vec3 thrust_acceleration = vec3(0);

    // player inventory
    [[SERIALIZE_POLY]]
    EntityPlayerInventory _inventory;

    // tool which player is "holding" (should be rendered)
    ItemRef held_tool;

    // TODO: different container?
    // essences
    Map<Essence, EssenceHolder> essences;

    EntityPlayer() = default;

    void on_before_deserialize(SerializationContextLevel &ctx);

    EntityPlayerInventory *inventory() override {
        return &this->_inventory;
    }

    const EntityPlayerInventory *inventory() const override {
        return &this->_inventory;
    }

    void init() override;

    void tick() override;

    void render(RenderContext &ctx) override;

    InteractionAffordance can_interact(
        InteractionKind kind,
        const Entity &target,
        ItemRef item) const override;

    InteractionResult interact(
        InteractionKind kind,
        Entity &target,
        ItemRef item) override;

    InteractionAffordance can_interact(
        InteractionKind kind,
        const ivec3 &pos,
        Direction::Enum face,
        ItemRef item) const override;

    InteractionResult interact(
        InteractionKind kind,
        const ivec3 &pos,
        Direction::Enum face,
        ItemRef item) override;

    LightArray lights() const override;

    vec3 hold_offset() const override;

    const ModelPlayer &model() const override;

    std::string name() const override {
        return Locale::instance()["entity:player"];
    }

    void on_pickup(const ItemStack &stack, EntityRef item) override;

    bool flying() const {
        return math::length(this->thrust_acceleration) > 0.0001f
            && !this->grounded();
    }

    // TODO: allow to be configurable on player creation
    Side::Enum side_tool() const {
        return Side::LEFT;
    }

    // TODO: allow to be configurable on player creation
    Side::Enum side_hold() const override {
        return Side::RIGHT;
    }

    // derived battery stat, containing max and current value
    Stat<f32> battery() const;

    // try to use "amount" of battery, returns false if it could not be used
    // checks across all available batteries
    bool try_use_battery(f32 amount);
};

using EntityPlayerRef = EntityPlayer::Ref;
