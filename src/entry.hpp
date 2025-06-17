#pragma once

#include "util/type_registry.hpp"

struct EntryManager {
    using MainFn = int(*)(int, char**);

    std::unordered_map<std::string, MainFn> entries;

    static EntryManager &instance();

    void add(const std::string &name, MainFn main) {
        this->entries[name] = main;
    }

    inline bool contains(const std::string &name) const {
        return this->entries.contains(name);
    }

    inline auto &operator[](const std::string &name) const {
        return this->entries.at(name);
    }
};

#define DECL_ENTRY_POINT(_n, _e)                                               \
    static auto CONCAT(_e, __COUNTER__) =                                      \
        (EntryManager::instance().add(#_n, _e), 0);
