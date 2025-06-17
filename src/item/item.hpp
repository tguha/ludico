#pragma once

#include "util/types.hpp"
#include "util/util.hpp"
#include "util/assert.hpp"
#include "util/direction.hpp"
#include "util/type_registry.hpp"
#include "util/alloc_ptr.hpp"
#include "entity/entity_ref.hpp"
#include "level/light.hpp"
#include "item/util.hpp"

struct Level;
struct Entity;
struct ItemStack;
struct ItemIcon;
struct ItemMetadata;
struct InteractionAffordance;
struct InteractionResult;
enum class InteractionKind : u8;
struct RenderState;
struct RenderContext;
struct UIItemDescriptionLine;

static constexpr auto MAX_ITEMS = 4096;

struct Item {
    enum class HoldType {
        DEFAULT = 0,
        DIAGONAL,
        TILE,
        ATTACHMENT,
        ATTACHMENT_FLIPPED
    };

    ItemId id;

    explicit Item(ItemId id) : id(id) {}
    virtual ~Item() = default;

    inline operator ItemId() const {
        return this->id;
    }

    // if true, item stacks are *expected* to have metadata on them
    virtual bool has_metadata() const {
        return false;
    }

    // gets the default metadata for this item
    // NOTE: must be implemented if has_metadata() is true
    virtual alloc_ptr<ItemMetadata> default_metadata(Level &level) const;

    virtual std::string name() const {
        return "(ERROR)";
    }

    virtual std::string locale_name() const {
        return "(ERROR)";
    }

    // if >1, item is considered stackable
    virtual usize stack_size() const {
        return 1;
    }

    // returns true if two stacks of this item are stackable
    // assumes that stack_size > 1 AND a.item() == b.item()
    virtual bool stackable(
        const ItemStack &a,
        const ItemStack &b) const {
        return true;
    }

    // optional extra item description lines
    virtual std::vector<std::tuple<std::string, vec4>> description(
        const ItemStack &stack) const {
        return {};
    }

    // check if this item held by entity can interact with target
    virtual InteractionAffordance can_interact(
        InteractionKind kind,
        const ItemStack &item,
        const Entity &entity,
        const Entity &target) const;

    // interact item held by entity with target
    virtual InteractionResult interact(
        InteractionKind kind,
        const ItemStack &item,
        Entity &entity,
        Entity &target) const;

    // check if this item held by entity can interact with tile at (pos, face)
    virtual InteractionAffordance can_interact(
        InteractionKind kind,
        const ItemStack &item,
        const Entity &entity,
        const ivec3 &pos,
        Direction::Enum face) const;

    // interact item held by entity with tile
    virtual InteractionResult interact(
        InteractionKind kind,
        const ItemStack &item,
        Entity &entity,
        const ivec3 &pos,
        Direction::Enum face) const;

    virtual LightArray lights(const ItemStack&) const {
        return LightArray();
    }

    virtual HoldType hold_type() const {
        return HoldType::DEFAULT;
    }

    // item hold offset, applied *after* (but before in matrix math) the
    // hold_type
    virtual vec3 hold_offset() const {
        return vec3(0);
    }

    // TODO: consider moving to a system where the icon value is generated on
    // each item (this function returns a std::unique_ptr<ItemIcon>) and then
    // the ptr is kept on Item()
    virtual const ItemIcon &icon() const = 0;

    // render item when held (fx. render block placement preview)
    virtual void render_held(
        RenderContext &ctx,
        const ItemStack &item,
        const Entity &entity,
        EntityRef target,
        const ivec3 &pos,
        Direction::Enum face) const;

    std::string to_string() const {
        return fmt::format(
            "Item({}, {}, {})",
            this->id,
            this->name(),
            this->locale_name());
    }

    template <typename T>
        requires std::is_base_of_v<Item, T>
    inline bool is() const {
        return dynamic_cast<const T*>(this) != nullptr;
    }

    template <typename T>
        requires std::is_base_of_v<Item, T>
    inline const T &as() const {
        auto *result = dynamic_cast<const T*>(this);
        ASSERT(result, "failed to cast to {}", typeid(T).name());
        return *result;
    }

    // retrieve an item type by its type index
    static const Item &by_type(const std::type_index &index);
};

template <typename T>
struct ItemType {
    static const T &get() {
        return dynamic_cast<const T&>(Item::by_type(typeid(T)));
    }
};

struct Items {
    static Items &get();

    void init();

    template <typename I>
    void register_type() {
        ItemId id;

        if constexpr (requires () { I::PRESET_ID; }) {
            LOG(
                "using preset item id {} for item {}",
                I::PRESET_ID,
                typeid(I).name());
            id = I::PRESET_ID;
        } else {
            id = this->next_id();
        }

        ASSERT(
            !this->items[id].get(),
            "while registering {}: item with id {} already present",
            typeid(I).name(),
            id);
        this->items[id] = std::make_unique<I>(id);
        this->by_index[std::type_index(typeid(I))] = this->items[id].get();
    }

    template <typename T>
    inline const T &get() const {
        return dynamic_cast<T&>(*this->by_index.at(std::type_index(typeid(T))));
    }

    inline const Item &get(const std::type_index &index) const {
        ASSERT(
            this->by_index.contains(index),
            "no registered item type {}",
            index.name());
        return dynamic_cast<const Item&>(*this->by_index.at(index));
    }

    inline const Item &operator[](ItemId id) const {
        return *this->items.at(id);
    }

private:
    inline ItemId next_id() {
        while (this->items[this->_next_id] && this->_next_id < MAX_ITEMS) {
            this->_next_id++;
        }
        ASSERT(this->_next_id != MAX_ITEMS, "ran out of item IDs");
        return this->_next_id;
    }

    ItemId _next_id = 1; // skip ItemAir
    std::array<std::unique_ptr<Item>, MAX_ITEMS> items;
    std::unordered_map<std::type_index, Item*> by_index;
};

using ItemRegistry = TypeRegistry<Items, Item>;

template <typename T>
using ItemRegistrar = TypeRegistrar<ItemRegistry, T>;

#define DECL_ITEM_REGISTER(_T)                                                 \
    static const auto CONCAT(_T, __COUNTER__) = (ItemRegistrar<_T>(), 1);
