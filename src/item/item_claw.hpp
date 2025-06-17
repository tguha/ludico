#pragma once

#include "item/item_tool.hpp"
#include "item/item_icon.hpp"
#include "item/item_metadata.hpp"
#include "util/time.hpp"
#include "locale.hpp"
#include "global.hpp"

struct ItemMetadataClaw : public ItemMetadata {
    using Base = ItemMetadata;
    using Base::Base;

    // only cosmetic
    u64 close_ticks = Time::TICK_MAX, close_time_ticks = 0;

    inline void close(u64 duration, u64 start = global.time->ticks) {
        this->close_ticks = start;
        this->close_time_ticks = duration;
    }

    inline bool is_open() const {
        return this->close_ticks == Time::TICK_MAX
            || (this->close_ticks + close_time_ticks) < global.time->ticks;
    }
};

struct ItemIconClaw : public ItemIconMeshed {
    using Base = ItemIconMeshed;

    enum Mesh {
        MESH_OPEN = 0,
        MESH_CLOSED,
        MESH_COUNT
    };

    ItemIconClaw(
        const TextureArea &area_open,
        const TextureArea &area_open_spec,
        const TextureArea &area_closed,
        const TextureArea &area_closed_spec);

    std::optional<TextureArea> texture_area(
        const ItemStack *stack) const override;

    const VoxelMultiMeshEntry &mesh(const ItemStack *stack) const override;

private:
    TextureArea area_open, area_open_spec, area_closed, area_closed_spec;
    std::array<const VoxelMultiMeshEntry*, MESH_COUNT> meshes;
};

struct ItemClaw
    : public ItemTool,
      public ItemMetadataType<ItemClaw, ItemMetadataClaw> {
    using Base = ItemTool;
    using Base::Base;

    std::string tool_name() const override {
        return "claw";
    }

    std::string tool_locale_name() const override {
        return Locale::instance()["item:claw"];
    }

    HoldType hold_type() const override {
        return HoldType::ATTACHMENT_FLIPPED;
    }

    vec3 hold_offset() const override {
        return vec3(-1, 4, 0);
    }

    bool has_metadata() const override {
        return true;
    }

    alloc_ptr<ItemMetadata> default_metadata(Level &level) const override;
};

struct ItemClawCopper
    : public ItemClaw,
      public ItemType<ItemClawCopper> {
    using Base = ItemClaw;
    using Base::Base;

    const ToolClass &tool_class() const override {
        return ToolClass::get(ToolClass::CLASS_COPPER);
    }

    const ItemIcon &icon() const override;
};
