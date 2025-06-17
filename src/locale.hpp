#pragma once

#include "util/assert.hpp"
#include "util/file.hpp"
#include "util/types.hpp"
#include "util/util.hpp"
#include "util/toml.hpp"
#include "util/log.hpp"

struct Locale {
    static Locale &instance();

    Locale() = default;
    explicit Locale(
        const std::unordered_map<std::string, std::string> &table)
        : table(table) {}

    const std::string &get(
        const std::string &name,
        const std::string &fallback = "MISSING NAME") const {
        const auto it = this->table.find(name);
        return it == table.end() ? fallback : it->second;
    }

    inline const auto &operator[](const std::string &key) const {
        ASSERT(table.contains(key), "No string \'{}\' in locale", key);
        return this->table.at(key);
    }

    static Locale from_toml(const std::string &path);

private:
    std::unordered_map<std::string, std::string> table;
};
