#include "item/item_battery.hpp"
#include "gfx/sprite_resource.hpp"
#include "gfx/specular_textures.hpp"
#include "level/level.hpp"
#include "serialize/serialize.hpp"

SERIALIZE_ENABLE()
DECL_SERIALIZER(ItemMetadataBattery)

static const auto spritesheet =
    SpritesheetResource("item_battery", "item/battery.png", { 8, 8 });

static usize index_for_charge(
    const ItemIconBattery &icon,
    const ItemBattery &item,
    f32 charge) {
    usize level = 0;

    if (charge > 1.0f) {
        level =
            charge / (item.charge_max() / ItemIconBattery::ICON_CHARGE_STAGES);
        level =
            math::min<usize>(
                level,
                ItemIconBattery::ICON_CHARGE_STAGES - 1);

        // level 0 is empty
        level += 1;
    }

    return level;
}

alloc_ptr<ItemMetadata> ItemBattery::default_metadata(Level &level) const {
    return make_alloc_ptr<Metadata>(level.item_metadata_allocator);
}

ItemIconBattery::ItemIconBattery(
    const ItemBattery &item,
    const TextureArea &base)
    : base(base) {
    this->sprites[0] = base;

    const auto &spritesheet_icon =
        TextureAtlas::get().alloc(
            fmt::format("item_icon_battery_{}", item.name()),
            { 8 * ICON_CHARGE_STAGES, 8 },
            { 8, 8 });

    for (usize i = 0; i < ICON_CHARGE_STAGES; i++) {
        const auto dst = spritesheet_icon[{ i, 0 }];

        // copy base sprite onto each entry
        pixels::copy(Pixels(dst), Pixels(base));

        // copy charge levels where alpha != 0
        pixels::copy_if(
            Pixels(dst),
            Pixels(spritesheet[uvec2(i, 0)]),
            [](const u32 &p) {
                const auto rgba =
                    PixelFormat::to_vec<vec4>(
                        TextureAtlas::PIXEL_FORMAT,
                        p);
                return rgba.a > math::epsilon<f32>();
            });

        this->sprites[i + 1] = dst;
    }

    spritesheet_icon.upload();

    // create meshes
    for (usize i = 0; i < this->sprites.size(); i++) {
        this->meshes[i] =
            &ItemIconMeshed::create_mesh(
                this->sprites[i],
                SpecularTextures::instance().min());
    }
}

std::optional<TextureArea> ItemIconBattery::texture_area(
    const ItemStack *stack) const {
    if (!stack) {
        return this->base;
    }

    const auto &item_battery = dynamic_cast<const ItemBattery&>(stack->item());
    const auto &metadata = stack->metadata<ItemMetadataBattery>();
    return this->sprites[
        index_for_charge(*this, item_battery, metadata.charge)];
}

const VoxelMultiMeshEntry &ItemIconBattery::mesh(const ItemStack *stack) const {
    if (!stack) {
        return *this->meshes[0];
    }

    const auto &item_battery = dynamic_cast<const ItemBattery&>(stack->item());
    const auto &metadata = stack->metadata<ItemMetadataBattery>();
    return *this->meshes[
        index_for_charge(*this, item_battery, metadata.charge)];
}

const ItemIcon &ItemBatteryCopper::icon() const {
    static auto icon = ItemIconBattery(*this, spritesheet[{ 0, 1 }]);
    return icon;
}

DECL_ITEM_REGISTER(ItemBatteryCopper);
