#include "item/item.hpp"
#include "item/item_metadata.hpp"
#include "item/item_icon.hpp"
#include "ui/ui_item_description.hpp"
#include "util/color.hpp"
#include "locale.hpp"

struct ItemBattery;

struct ItemMetadataBattery : public ItemMetadata {
    f32 charge = 0;

    std::string to_string() const override {
        return fmt::format("ItemBatteryMetadata({})", charge);
    }
};

struct ItemIconBattery : public ItemIconMeshed {
    static constexpr auto ICON_CHARGE_STAGES = 4;

    TextureArea base;

    std::array<TextureArea, 1 + ICON_CHARGE_STAGES> sprites;
    std::array<const VoxelMultiMeshEntry*, 1 + ICON_CHARGE_STAGES> meshes;

    ItemIconBattery(
        const ItemBattery &item,
        const TextureArea &base);

    std::optional<TextureArea> texture_area(
        const ItemStack *stack) const override;

    const VoxelMultiMeshEntry &mesh(const ItemStack *stack) const override;
};

// batteries should give off light according to their current charge level
struct ItemBattery
    : public virtual Item,
      public ItemMetadataType<ItemBattery, ItemMetadataBattery> {
    using Base = Item;
    using Base::Base;

    static constexpr auto
        LEVEL_MIN = 0,
        LEVEL_MAX = 4;

    std::vector<std::tuple<std::string, vec4>> description(
        const ItemStack &stack) const override {
        return {{
            fmt::format(
                "{:.0f}/{:.0f}",
                stack.metadata<ItemMetadataBattery>().charge,
                this->charge_max()),
            color::argb32_to_v(0xff00ffff)
        }};
    }

    bool has_metadata() const override {
        return true;
    }

    alloc_ptr<ItemMetadata> default_metadata(Level &level) const override;

    std::string name() const override {
        return fmt::format("battery_{}", this->battery_name());
    }

    std::string locale_name() const override {
        return fmt::vformat(
            Locale::instance()["item:battery_fmt"],
            fmt::make_format_args(this->battery_locale_name()));
    }

    virtual std::string battery_name() const = 0;

    virtual std::string battery_locale_name() const = 0;

    virtual f32 charge_max() const = 0;
};

struct ItemBatteryCopper
    : public virtual ItemBattery,
      public ItemType<ItemBatteryCopper> {
    using Base = ItemBattery;
    using Base::Base;

    explicit ItemBatteryCopper(ItemId id)
        : Item(id),
          ItemBattery(id) {}

    std::string battery_name() const override {
        return "battery_copper";
    }

    std::string battery_locale_name() const override {
        return Locale::instance()["item:battery_copper"];
    }

    f32 charge_max() const override {
        return 100.0f;
    }

    const ItemIcon &icon() const override;
};
