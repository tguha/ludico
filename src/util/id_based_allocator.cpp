#include "util/id_based_allocator.hpp"
#include "util/optional.hpp"
#include "util/collections.hpp"

#include <archimedes.hpp>

void IDBasedAllocator::set(arc::type_id id, GetFn &&fn) {
    const auto ref =
        OPT_OR_ASSERT(
            arc::reflect(id),
            "could not reflect id {}",
            id.value());

    // no set if not record
    if (!ref.is_record()) {
        this->sets.emplace_back(
            std::unordered_set<arc::type_id> { id }, std::move(fn));
        return;
    }

    const auto children = arc::reflect_children(ref.as_record());
    std::unordered_set<arc::type_id> ids;
    collections::copy_to(
        children, ids, [](const auto &rec) { return rec.id(); });
    this->sets.emplace_back(ids, std::move(fn));
}
