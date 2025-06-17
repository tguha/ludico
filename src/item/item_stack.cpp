#include "item/item_stack.hpp"
#include "item/item.hpp"
#include "level/level.hpp"
#include "serialize/serialize.hpp"

SERIALIZE_ENABLE()
DECL_SERIALIZER_OPT(ItemStack)
DECL_PRIMITIVE_SERIALIZER_OPT(alloc_ptr<ItemMetadata>)

ItemStack::ItemStack(Level &level, const Item &item, usize size)
    : size(size),
      _level(&level),
      _item(&item) {
    this->id = this->level().next_item_stack_id();

    if (item.has_metadata()) {
        this->add_metadata(item.default_metadata(level));
    }
}

std::string ItemStack::to_string() const {
    return fmt::format(
        "ItemStack({},{},{})",
        this->id,
        this->item(),
        this->_metadata);
}
