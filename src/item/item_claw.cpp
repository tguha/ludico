#include "item/item_claw.hpp"
#include "item/item_icon.hpp"
#include "level/level.hpp"
#include "gfx/sprite_resource.hpp"
#include "gfx/specular_textures.hpp"
#include "serialize/serialize.hpp"

DECL_ITEM_REGISTER(ItemClawCopper)

SERIALIZE_ENABLE()
DECL_SERIALIZER(ItemMetadataClaw)

static const auto spritesheet =
    SpritesheetResource("item_claw", "item/claw.png", { 8, 8 });

alloc_ptr<ItemMetadata> ItemClaw::default_metadata(Level &level) const {
    return make_alloc_ptr<ItemMetadataClaw>(level.item_metadata_allocator);
}

ItemIconClaw::ItemIconClaw(
    const TextureArea &area_open,
    const TextureArea &area_open_spec,
    const TextureArea &area_closed,
    const TextureArea &area_closed_spec)
    : Base(),
      area_open(area_open),
      area_open_spec(area_open_spec),
      area_closed(area_closed),
      area_closed_spec(area_closed_spec) {
    this->meshes[MESH_OPEN] =
        &ItemIconMeshed::create_mesh(this->area_open, this->area_open_spec);
    this->meshes[MESH_CLOSED] =
        &ItemIconMeshed::create_mesh(this->area_closed, this->area_closed_spec);
}

std::optional<TextureArea> ItemIconClaw::texture_area(
    const ItemStack *stack) const {
    if (!stack) {
        return this->area_open;
    }

    return
        ItemClaw::get_metadata(*stack).is_open() ?
            this->area_open
            : this->area_closed;
}

const VoxelMultiMeshEntry &ItemIconClaw::mesh(const ItemStack *stack) const {
    return *this->meshes[
        (!stack || ItemClaw::get_metadata(*stack).is_open()) ?
            MESH_OPEN
            : MESH_CLOSED];
}

const ItemIcon &ItemClawCopper::icon() const {
    static auto icon =
        ItemIconClaw(
            spritesheet[{ 0, 0 }],
            SpecularTextures::instance().min(),
            spritesheet[{ 1, 0 }],
            SpecularTextures::instance().min());
    return icon;
}
