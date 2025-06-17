#pragma once

#include <archimedes/type_id.hpp>
namespace arc = archimedes;

#include "util/util.hpp"
#include "util/types.hpp"
#include "util/allocator.hpp"

// TODO: overlap in sets, pick based on more specific type?
// TODO: doc
struct IDBasedAllocator {
    using GetFn = std::function<Allocator&(void)>;

    IDBasedAllocator() = default;
    explicit IDBasedAllocator(Allocator *default_allocator)
        : default_allocator(default_allocator) {}

    template <typename T, typename F>
    IDBasedAllocator &set(F &&f) {
        this->set(arc::type_id::from<T>(), GetFn(std::forward<F>(f)));
        return *this;
    }

    Allocator &allocator(arc::type_id id) const {
        usize n = NO_INDEX;

        auto it = this->cache.find(id);
        if (it == this->cache.end()) {
            n = this->find(id);
            this->cache[id] = n;
        } else {
            n = it->second;
        }

        return n == NO_INDEX ?
            *this->default_allocator
            : std::get<1>(this->sets[n])();
    }

private:
    static constexpr auto NO_INDEX = std::numeric_limits<usize>::max();

    usize find(arc::type_id id) const {
        for (usize i = 0; i < this->sets.size(); i++) {
            const auto &[s, _] = this->sets[i];

            if (s.contains(id)) {
                return i;
            }
        }

        return NO_INDEX;
    }

    void set(arc::type_id id, GetFn &&fn);

    Allocator *default_allocator;
    mutable std::unordered_map<arc::type_id, usize> cache;
    std::vector<std::tuple<std::unordered_set<arc::type_id>, GetFn>> sets;
};
