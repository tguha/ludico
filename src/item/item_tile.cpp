#include "item/item_tile.hpp"
#include "constants.hpp"
#include "entity/entity_mob.hpp"
#include "gfx/render_context.hpp"
#include "level/level.hpp"
#include "tile/tile.hpp"
#include "tile/tile_renderer.hpp"

ItemTile::ItemTile(ItemId id, const Tile &tile)
    : Item(id),
      _tile(&tile),
      _icon(tile) {}

std::string ItemTile::name() const {
    return this->tile().name();
}

std::string ItemTile::locale_name() const {
    return this->tile().locale_name();
}

InteractionAffordance ItemTile::can_interact(
    InteractionKind kind,
    const ItemStack &item,
    const Entity &entity,
    const ivec3 &pos,
    Direction::Enum face) const {
    const auto can =
        kind == InteractionKind::SECONDARY
            && item.level().can_place_tile(
                this->tile(),
                pos + Direction::to_ivec3(face));
    return
        InteractionAffordance(
            can ? InteractionAffordance::OVERRIDE : InteractionAffordance::NONE,
            0.5f);
}

InteractionResult ItemTile::interact(
    InteractionKind kind,
    const ItemStack &item,
    Entity &entity,
    const ivec3 &pos,
    Direction::Enum face) const {
    if (const auto mob = EntityRef(entity).as<EntityMob>()) {
        if (auto *inventory = mob->inventory()) {
            inventory->remove(item, 1);
        }
    }

    item.level().tiles[pos + Direction::to_ivec3(face)] = this->tile();
    return InteractionResult::SUCCESS;
}

void ItemTile::render_held(
    RenderContext &ctx,
    const ItemStack &item,
    const Entity &entity,
    EntityRef target,
    const ivec3 &pos,
    Direction::Enum face) const {
    // only entity interactions
    if (target) {
        return;
    }

    // only if can_interact
    if (!this->can_interact(
            InteractionKind::SECONDARY,
            item,
            entity,
            pos,
            face).positive()) {
        return;
    }

    EntityMob::Ref mob;
    if (!(mob = EntityRef(entity).as<EntityMob>())) {
        return;
    }

    if (!mob->can_reach(
            Level::to_tile_center(pos + Direction::to_ivec3(face)))) {
        return;
    }

    // TODO: cleanup
    auto &group = ctx.push_group();
    {
        // only render on transparent pass!
        group.group_passes = RENDER_FLAG_PASS_TRANSPARENT;

        ctx.push(
            RenderCtxFn {
                [this, pos, face, item = &item](
                    const RenderGroup&,
                    RenderState render_state) {
                    TileRenderer::render(
                        this->tile(),
                        &item->level(),
                        pos + Direction::to_ivec3(face),
                        render_state,
                        vec4(0.0f),
                        math::abs(math::time_wave(2.0f)));
                }});
    }
    ctx.pop_group();
}
