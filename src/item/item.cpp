#include "item/item.hpp"
#include "entity/interaction.hpp"
#include "gfx/util.hpp"

template<> ItemRegistry &ItemRegistry::instance() {
    static ItemRegistry instance;
    return instance;
}

Items &Items::get() {
    static Items items;
    return items;
}

void Items::init() {
    ItemRegistry::instance().initialize(*this);
}

alloc_ptr<ItemMetadata> Item::default_metadata(Level &level) const {
    return {};
}

void Item::render_held(
    RenderContext &ctx,
    const ItemStack &item,
    const Entity &entity,
    EntityRef target,
    const ivec3 &pos,
    Direction::Enum face) const {}

InteractionAffordance Item::can_interact(
    InteractionKind kind,
    const ItemStack &item,
    const Entity &entity,
    const Entity &target) const {
    return InteractionAffordance::NONE;
}

InteractionResult Item::interact(
    InteractionKind kind,
    const ItemStack &item,
    Entity &entity,
    Entity &target) const {
    return InteractionResult::NONE;
}

InteractionAffordance Item::can_interact(
    InteractionKind kind,
    const ItemStack &item,
    const Entity &entity,
    const ivec3 &pos,
    Direction::Enum face) const {
    return InteractionAffordance::NONE;
}

InteractionResult Item::interact(
    InteractionKind kind,
    const ItemStack &item,
    Entity &entity,
    const ivec3 &pos,
    Direction::Enum face) const {
    return InteractionResult::FAILURE;
}

const Item &Item::by_type(const std::type_index &index) {
    return Items::get().get(index);
}
