#include "entity/entity_item.hpp"
#include "entity/entity_plant.hpp"
#include "entity/entity_player.hpp"
#include "entity/entity_smoke_particle.hpp"
#include "entity/interaction.hpp"
#include "gfx/game_camera.hpp"
#include "gfx/renderer_resource.hpp"
#include "input/controls.hpp"
#include "item/item.hpp"
#include "item/item_battery.hpp"
#include "item/item_claw.hpp"
#include "item/item_hammer.hpp"
#include "model/model_player.hpp"
#include "platform/platform.hpp"
#include "sound/sound_resource.hpp"
#include "state/state_game.hpp"
#include "tile/tile_stone.hpp"
#include "tile/tile_essence_tank.hpp"
#include "ui/ui_hud.hpp"
#include "ui/ui_inventory.hpp"
#include "ui/ui_root.hpp"
#include "util/color.hpp"
#include "util/side.hpp"
#include "util/time.hpp"
#include "serialize/serialize.hpp"
#include "serialize/context_level.hpp"
#include "global.hpp"

SERIALIZE_ENABLE()
DECL_PRIMITIVE_SERIALIZER_OPT(EntityPlayer::BlinkerState)
DECL_SERIALIZER(EntityPlayerInventory)
DECL_SERIALIZER(EntityPlayerInventory)
DECL_SERIALIZER(EntityPlayer)

static const auto model_manager =
    RendererResource<ModelPlayer>(
        []() { return ModelPlayer(); });

static const auto sound_footstep =
    SoundResource("sound/footstep0.wav");

static const auto sound_pickup =
    SoundResource("sound/pickup.wav");

EntityPlayerInventory::EntityPlayerInventory(const EntityPlayer &player)
    : Base(*player.level, SIZE, player) {}

bool EntityPlayerInventory::allowed(usize i, const Item &item) const {
    if (RANGE_NON_HIDDEN.contains(i)) {
        return true;
    } else if (RANGE_BATTERY_GRID.contains(i)) {
        return types::instance_of<ItemBattery>(item);
    } else if (RANGE_GENERATOR.contains(i)) {
        return true; // TODO
    } else if (RANGE_LOGIC_BOARD.contains(i)) {
        return true; // TODO
    } else if (RANGE_TOOLS.contains(i)) {
        if (i - RANGE_TOOLS.min == 0) {
            return types::instance_of<ItemHammer>(item);
        } else {
            return types::instance_of<ItemClaw>(item);
        }
    }


    ASSERT(false, "index {} out of range", i);
    return false;
}

void EntityPlayer::on_before_deserialize(SerializationContextLevel &ctx) {
    this->essences.allocator = &ctx.level->allocator;
}

void EntityPlayer::init() {
    Base::init();
    this->drag = 1.25f;

    this->_inventory = EntityPlayerInventory(*this);
    this->inventory()->try_add(
        ItemStack(*this->level, Items::get().get<ItemStone>(), 4));

    this->inventory()->try_add(
        ItemStack(*this->level, Items::get().get<ItemEssenceTank>(), 4));

    const auto [stack, _] =
        this->inventory()->try_add(
            ItemStack(*this->level, ItemBatteryCopper::get(), 1),
            this->inventory()->batteries()[0]->index());
    ItemBatteryCopper::get().metadata(*stack).charge = 40.0f;

    this->inventory()->try_add(
        ItemStack(*this->level, ItemHammerCopper::get(), 1),
        this->inventory()->hammer().index());

    this->inventory()->try_add(
        ItemStack(*this->level, ItemClawCopper::get(), 1),
        this->inventory()->claw().index());

    // start by holding claw
    this->held_tool = ItemRef::from_slot(this->inventory()->claw());

    this->essences = decltype(this->essences)(&this->level->allocator);
    this->essences.insert(
        Essence::WATER,
        EssenceHolder {
            .stack = EssenceStack { .kind = Essence::FIRE, .amount = 44 },
            .max = 100
        });
}

static void apply_controls(EntityPlayer &entity) {
    vec2 movement(0);

    if (global.controls->up.is_down()) {
        movement += vec2(1.0f, 1.0f);
    }

    if (global.controls->down.is_down()) {
        movement += vec2(-1.0f, -1.0f);
    }

    if (global.controls->right.is_down()) {
        movement += vec2(1.0f, -1.0f);
    }

    if (global.controls->left.is_down()) {
        movement += vec2(-1.0f, 1.0f);
    }

    // adjust to current rotation
    switch (global.game->camera->rotation) {
        case GameCamera::ROT_SE: break;
        case GameCamera::ROT_SW: movement = movement.yx() * vec2(-1, 1); break;
        case GameCamera::ROT_NW: movement *= vec2(-1, -1); break;
        case GameCamera::ROT_NE: movement = movement.yx() * vec2(1, -1); break;
        default:
            ENOIMPL();
    }

    // apply adjusted movement, adjust by speed and potential air drag
    if (math::length(movement) > math::epsilon<f32>()) {
        const auto speed = entity.speed() * EntityMob::SPEED_BY_TICKS;
        movement = math::normalize(movement);

        auto xz_modifier = vec2(1.0);

        if (!entity.grounded()) {
            xz_modifier *= vec2(0.6f);
        }

        movement *= xz_modifier;

        entity.velocity.x += movement.x * speed;
        entity.velocity.z += movement.y * speed;
    }

    // handle vertical movement via thrust
    if (global.controls->jump.is_down()) {
        if (*entity.charge > EntityPlayer::FLIGHT_CHARGE_COST) {
            entity.charge -= EntityPlayer::FLIGHT_CHARGE_COST;
            entity.thrust_acceleration =
                math::min(
                    entity.thrust_acceleration + EntityPlayer::THRUST_POWER,
                    EntityPlayer::THRUST_MAX);
            entity.velocity += entity.thrust_acceleration;
        }
    }

    // drop one of held item
    if (global.controls->drop.is_pressed_tick()) {
        if (entity.held()) {
            auto stack = entity.inventory()->remove(*entity.held(), 1);
            EntityItem::drop_from_entity(entity, std::move(stack));
        }
    }
}

// updates the current angle the player is facing according to mouse movement,
// direction of walking, etc.
static void update_facing_angle(EntityPlayer &entity) {
    constexpr auto
        ACTION_FADE_DECAY_TICKS = 40,
        MOUSE_LOOK_DELAY_TICKS = 120;

    // turn towards current movement direction
    if (math::length(entity.velocity) > math::epsilon<f32>()) {
        entity.facing = entity.velocity;
    }

    const auto inventory_look =
        global.game->hud->inventory->is_enabled()
            && entity.last_move_ticks < (global.time->ticks - 1);

    // inventory is open OR haven't moved in a bit? turn towards mouse
    if (inventory_look
        || global.time->ticks >
            (entity.last_move_ticks + MOUSE_LOOK_DELAY_TICKS)) {
        if (const auto mouse_tile = global.game->camera->mouse_tile()) {
            const auto [_0, _1, hit] = *mouse_tile;
            entity.facing = math::normalize(hit - entity.center);

            // force facing direction if inventory look
            if (inventory_look) {
                return;
            }
        }
    }

    // turn towards clicked things
    std::optional<Interaction> in;
    in = entity.current_interaction;
    in = in ? in : entity.last_interaction;

    if (in) {
        if (in->ticks + ACTION_FADE_DECAY_TICKS >= global.time->ticks) {
            entity.facing = math::normalize(in->pos - entity.center);
        }
    }
}

using InteractFn = std::function<void(const Interaction&)>;

struct ChargingInteraction {
    Interaction i;
    f32 c;
    InteractFn f;
};

// tries to interact the player with the specified entity
static std::optional<ChargingInteraction> try_interact_entity(
    EntityPlayer &entity,
    InteractionKind kind,
    Entity &target) {
    if (!entity.can_reach(target.center)) {
        return std::nullopt;
    }

    const auto item = entity.held();
    const auto [affordances, charge] =
        TRY_OPT(
            InteractionAffordance::combine(
                types::make_array(
                    entity.can_interact(
                        kind,
                        target,
                        item),
                    target.can_interact_with_this(
                        kind,
                        entity,
                        item),
                    item == ItemRef::none() ?
                        InteractionAffordance::NONE
                        : item->item().can_interact(
                            kind,
                            *item,
                            entity,
                            target))));

    const auto in =
        Interaction(
            kind,
            target.center,
            entity.held(),
            entity,
            target,
            InteractionAffordance::get_tool(affordances));

    const auto fn =
        [affordances = affordances](const Interaction &i) {
            const auto
                &aff_entity_target = affordances[0],
                &aff_target_entity = affordances[1],
                &aff_item = affordances[2];

            if (aff_entity_target.positive()) {
                i.source->interact(
                    i.kind,
                    *i.target,
                    i.stack);
            }

            if (aff_target_entity.positive()) {
                i.target->interacted(
                    InteractionKind::PRIMARY,
                    *i.source,
                    i.stack);
            }

            if (aff_item.positive() && i.stack) {
                i.stack->item().interact(
                    i.kind,
                    *i.stack,
                    *i.source,
                    *i.target);
            }
        };

    return ChargingInteraction { in, charge, fn };
}

// tries to interact the player with the specified tile
static std::optional<ChargingInteraction> try_interact_tile(
    EntityPlayer &entity,
    InteractionKind kind,
    const ivec3 &pos,
    Direction::Enum face) {
    // TODO: differentiate between reach when placing a tile (interacting with
    // empty space) and interacting with a tile
    if (!entity.can_reach(Level::to_tile_center(pos))) {
        return std::nullopt;
    }

    const auto item = entity.held();
    const auto &tile = Tiles::get()[entity.level->tiles[pos]];
    const auto [affordances, charge] =
        TRY_OPT(
            InteractionAffordance::combine(
                types::make_array(
                    // TODO: bug? without cast, clang claims that a call to
                    // EntityMob::can_interact is needed :/
                    dynamic_cast<Entity&>(entity).can_interact(
                        kind,
                        pos,
                        face,
                        item),
                    tile.can_interact_with_this(
                        *entity.level,
                        pos,
                        face,
                        kind,
                        entity,
                        item),
                    item == ItemRef::none() ?
                        InteractionAffordance::NONE
                        : item->item().can_interact(
                            kind,
                            *item,
                            entity,
                            pos,
                            face))));

    const auto in =
        Interaction(
            kind,
            Level::to_tile_center(pos),
            item,
            entity,
            pos,
            face,
            InteractionAffordance::get_tool(affordances));

    const auto fn =
        [affordances = affordances, &tile = tile](const Interaction &i) {
            const auto
                &aff_entity_tile = affordances[0],
                &aff_tile_entity = affordances[1],
                &aff_item = affordances[2];

            if (aff_entity_tile.positive()) {
                i.source->interact(
                    i.kind,
                    *i.tile,
                    *i.face,
                    i.stack);
            }

            if (aff_tile_entity.positive()) {
                tile.interacted(
                    *i.source.level,
                    *i.tile,
                    *i.face,
                    i.kind,
                    *i.source,
                    i.stack);
            }

            if (aff_item.positive() && i.stack) {
                i.stack->item().interact(
                    i.kind,
                    *i.stack,
                    *i.source,
                    *i.tile,
                    *i.face);
            }
        };

    return ChargingInteraction { in, charge, fn };
}

// tries to perform a potentially charged action
static void try_perform(EntityPlayer &entity, const ChargingInteraction &ir) {
    const auto &current = entity.current_interaction;
    const auto &[in, charge, fn] = ir;

    if (!current || !in.same(*current)) {
        entity.interaction_charge = 0.0f;
        entity.current_interaction = in;
        return;
    }

    // not enough charge!
    if (entity.interaction_charge < charge) {
        return;
    }

    // perform the interaction
    fn(in);
    entity.interaction_charge = 0.0f;
    entity.last_interaction = in;
}

// tries to interact the player with the world according to input states
static void update_interaction(EntityPlayer &entity) {
    // cancel interaction if ui has mouse
    if (global.ui->has_mouse()) {
        entity.interaction_charge = 0.0f;
        entity.current_interaction = std::nullopt;
        return;
    }

    // choose the interaction which was least recently pressed
    const auto
        &primary = global.controls->primary,
        &secondary = global.controls->secondary;

    const auto last_interaction_ticks =
        entity.last_interaction ?
            entity.last_interaction->ticks
            : 0;

    std::optional<InteractionKind> kind_opt;
    if (primary.is_down()
            && primary.tick_down() <= secondary.tick_down()
            && primary.tick_down() >= last_interaction_ticks) {
        kind_opt = InteractionKind::PRIMARY;
    } else if (secondary.is_down()
                && secondary.tick_down() <= primary.tick_down()
                && secondary.tick_down() >= last_interaction_ticks) {
        kind_opt = InteractionKind::SECONDARY;
    }

    if (!kind_opt) {
        entity.interaction_charge = 0.0f;
        entity.current_interaction = std::nullopt;
        return;
    }

    const auto kind = *kind_opt;

    std::optional<ChargingInteraction> ir;

    // try to interact with entity
    const auto mouse_entity = global.game->camera->mouse_entity();
    if (mouse_entity) {
        ir = try_interact_entity(entity, kind, *mouse_entity);
    }

    // try to interact with tile
    const auto mouse_tile = global.game->camera->mouse_tile();
    if (!ir) {
        const auto [pos, face, _] = *mouse_tile;
        ir = try_interact_tile(entity, kind, pos, face);
    }

    // switch tool to whichever is used:
    // * PRIMARY -> hammer
    // * SECONDARY -> claw
    entity.held_tool =
        ir && ir->i.tool ?
            ItemRef::from_slot(
                *ir->i.tool == ToolKind::HAMMER ?
                    entity.inventory()->hammer()
                    : entity.inventory()->claw())
            : entity.held_tool;

    // add charge and try perform action if present
    if (ir) {
        if (entity.charge_penalty_ticks == 0
                && *entity.charge >= EntityPlayer::INTERACTION_CHARGE_RATE) {
            entity.charge -= EntityPlayer::INTERACTION_CHARGE_RATE;
            entity.interaction_charge += EntityPlayer::INTERACTION_CHARGE_RATE;
        }

        try_perform(entity, *ir);
    } else {
        entity.interaction_charge = 0.0f;
    }
}

static void update_charge(EntityPlayer &entity) {
    if (*entity.charge < 0.05f
            && entity.charge_penalty_ticks == 0) {
        entity.charge_penalty_ticks = EntityPlayer::CHARGE_ZERO_PENALTY_TICKS;
    } else if (entity.charge_penalty_ticks > 0) {
        entity.charge_penalty_ticks--;
    }

    if (entity.charge_penalty_ticks == 0
            && !entity.current_interaction
            && *entity.charge < entity.charge.max()
            && entity.charge.last_change_ticks()
                <= global.time->ticks - EntityPlayer::RECHARGE_WAIT_TICKS) {
        // set silently so that last_change_ticks does not change
        if (entity.try_use_battery(EntityPlayer::CHARGE_RECHARGE_RATE)) {
            entity.charge.set(
                entity.charge + EntityPlayer::CHARGE_RECHARGE_RATE, true);
        }
    }
}

static void update_animation(EntityPlayer &entity) {
    // handle thrust animation
    if (entity.flying()) {
        // TODO: don't use magic constant for max velocity calculation
        entity.animation_state.tilt_body = KeyframeList<f32> {
            {
                {
                    25,
                    (math::PI / 10.0f) *
                        (math::length(entity.velocity.xz()) / 0.012f),
                },
            },
            entity.animation_state.tilt_body.as_keyframe()
        };

        for (const auto &side : Side::ALL) {
            entity.animation_state.rot_legs[side] = {
                {
                    { 25, 0.0f }
                },
                entity.animation_state.rot_legs[side].as_keyframe()
            };
        }
    } else {
        entity.animation_state.tilt_body = {
            {{ 25, 0.0f }},
            entity.animation_state.tilt_body.as_keyframe()
        };
    }

    // swing
    entity.animation_state.swing =
        entity.flying() ?
            std::nullopt
            : std::make_optional(ModelHumanoid::swing(entity));

    // head rotation
    entity.animation_state.rot_head =
        {{ 0, entity.facing.xz_angle_lerp16() }};
}

void EntityPlayer::tick() {
    Base::tick();
    apply_controls(*this);
    update_facing_angle(*this);
    update_interaction(*this);
    update_charge(*this);
    update_animation(*this);

    // control blinker state
    if (*this->battery() < LOW_POWER_THRESHOLD) {
        this->blinker_state = BlinkerState::LOW_POWER;
    } else if (this->flying()) {
        this->blinker_state = BlinkerState::FLIGHT;
    } else {
        this->blinker_state = BlinkerState::DEFAULT;
    }

    // control footsteps
    if (math::length(this->velocity) > 0.015f
        && this->grounded()) {
        const auto
            rot_left =
                this->model()
                    .rot_swing_leg(
                        *this,
                        this->animation_state,
                        Side::LEFT),
            rot_right =
                this->model()
                    .rot_swing_leg(
                        *this,
                        this->animation_state,
                        Side::RIGHT);
        this->footstep =
            !this->footstep
                && (rot_left < 0.00001f || rot_right < 0.00001f);
    } else {
        this->footstep = false;
    }

    if (this->footstep) {
        sound_footstep->play();
    }

    // apply thrust drag
    this->thrust_acceleration *= EntityPlayer::THRUST_DRAG;
    if (math::length(this->thrust_acceleration) < 0.001f) {
        this->thrust_acceleration = vec3(0.0f);
    }
}

static void render_held(
    const EntityPlayer &entity,
    RenderContext &ctx,
    const ItemStack &held) {
    if (global.ui->has_mouse()) {
        return;
    }

    if (const auto mouse_tile = global.game->camera->mouse_tile()) {
        const auto [pos, face, _] = *mouse_tile;
        held.item().render_held(
            ctx,
            held,
            entity,
            global.game->camera->mouse_entity(),
            pos,
            face);
    }
}

void EntityPlayer::render(RenderContext &ctx) {
    Base::render(ctx);

    model_manager->render(
        ctx,
        {
            .entity = this,
            .anim = &this->animation_state
        });

    if (auto held = this->held()) {
        render_held(*this, ctx, *held);
    }
}

InteractionAffordance EntityPlayer::can_interact(
    InteractionKind kind,
    const Entity &target,
    ItemRef item) const {
    return kind == InteractionKind::PRIMARY &&
        dynamic_cast<const EntityPlant*>(&target) != nullptr ?
        InteractionAffordance(
            InteractionAffordance::YES,
            1.0f,
            ToolKind::CLAW)
        : InteractionAffordance::NONE;
}

InteractionResult EntityPlayer::interact(
    InteractionKind kind,
    Entity &target,
    ItemRef item) {
    // animate when plants are harvested
    if (EntityRef(target).as<EntityPlant>()) {
        ModelHumanoid::animate_grab(this->animation_state);

        // briefly close claw if present
        if (const auto claw = ItemRef::from_slot(this->inventory()->claw())) {
            ItemClaw::get_metadata(*claw).close(30);
        }
    }

    return InteractionResult::SUCCESS;
}

InteractionAffordance EntityPlayer::can_interact(
    InteractionKind kind,
    const ivec3 &pos,
    Direction::Enum face,
    ItemRef item) const {
    return kind == InteractionKind::PRIMARY ?
        InteractionAffordance(
            InteractionAffordance::YES,
            1.0f,
            ToolKind::HAMMER)
        : InteractionAffordance::NONE;
}

InteractionResult EntityPlayer::interact(
    InteractionKind kind,
    const ivec3 &pos,
    Direction::Enum face,
    ItemRef item) {
    if (kind == InteractionKind::PRIMARY) {
        ModelHumanoid::animate_smash(this->animation_state, this->side_tool());
        return InteractionResult::SUCCESS;
    }

    return Base::interact(kind, pos, face, item);
}

static std::optional<Light> get_blinker_light(const EntityPlayer &entity) {
    vec3 color;
    f32 freq = 0.0f;

    switch (entity.blinker_state) {
        case EntityPlayer::BlinkerState::DEFAULT:
            color = color::argb32_to_v(0xff3fbf3f).rgb() * 4.0f;
            freq = 1.5f;
            break;
        case EntityPlayer::BlinkerState::LOW_POWER:
            color = color::argb32_to_v(0xffff3333).rgb() * 8.0f;
            freq = 6.0f;
            break;
        case EntityPlayer::BlinkerState::FLIGHT:
            color = color::argb32_to_v(0xffffff3f).rgb() * 6.0f,
            freq = 4.8f;
            break;
        default:
            break;
    }

    if (freq < math::epsilon<f32>()
        || (global.time->ticks / usize(TICKS_PER_SECOND / freq)) % 2 == 0) {
        return std::nullopt;
    }

    auto l = Light(
        math::xyz_to_xnz(entity.center, entity.aabb()->max.y + 0.1f),
        color,
        2,
        false);
    l.att_quadratic = 40.0f;
    return l;
}

LightArray EntityPlayer::lights() const {
    auto arr = Base::lights();
    if (const auto blinker = get_blinker_light(*this)) {
        arr.try_add(*blinker);
    }
    return arr;
}

vec3 EntityPlayer::hold_offset() const {
    const auto model_hold =
        model_manager->model_hold(
            *this,
            this->animation_state,
            model_manager->model_base(
                *this,
                this->animation_state),
            this->side_hold(),
            this->held());
    return
        (model_hold * vec4(vec3(0.0f, 0.4f, 0.0f), 1.0f)).xyz() - this->pos;
}

const ModelPlayer &EntityPlayer::model() const {
    return model_manager.get();
}

void EntityPlayer::on_pickup(const ItemStack &stack, EntityRef item) {
    if (item) {
        sound_pickup->play();
    }

    global.game->hud->item_description->set(
        UIItemDescription::Entry(stack, 60));
}

Stat<f32> EntityPlayer::battery() const {
    f32 value = 0.0f, max = 0.0f;
    for (const auto *slot : this->inventory()->batteries()) {
        if (slot->empty()) {
            continue;
        }

        const auto &item_battery = (*slot)->item().as<ItemBattery>();
        value += item_battery.metadata(**slot).charge;
        max += item_battery.charge_max();
    }

    return Stat<f32>(value, 0.0f, max);
}

bool EntityPlayer::try_use_battery(f32 amount) {
    // list of batteries to take charge from in what amounts
    std::vector<std::tuple<ItemStack*, f32>> from;

    for (auto *slot : this->inventory()->batteries()) {
        if (slot->empty()) {
            continue;
        }

        auto &metadata = ItemBattery::get_metadata(**slot);
        if (metadata.charge > 0.0f) {
            const auto v = math::min(metadata.charge, amount);
            from.push_back({ &**slot, v });
            amount -= v;
        }
    }

    // if amount is not zero, could not gather enough battery
    if (amount > 0.0f) {
        return false;
    }

    // take charge
    for (const auto &[stack, v] : from) {
        ItemBattery::get_metadata(*stack).charge -= v;
    }

    return true;
}
