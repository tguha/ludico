#include "item/item_ref.hpp"
#include "item/item_container.hpp"
#include "serialize/serialize.hpp"

SERIALIZE_ENABLE()
DECL_SERIALIZER(ItemRef)

ItemRef::operator bool() const {
    return this->_parent
        && this->_parent->index_of(this->id()) != -1;
}

ItemStack &ItemRef::operator*() const {
    return *(*this->_parent)[this->_parent->index_of(this->id())];
}
