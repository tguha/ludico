#pragma once

#include "util/log.hpp"

#include <toml.hpp>

namespace toml {
template <typename M>
M as_map(const toml::table &tbl, M &&init, const std::string &sep = ":") {
    auto map = std::move(init);

    using VisitFn = std::function<void(const std::string&, const toml::table&)>;
    VisitFn visit;

    visit = [&](const std::string &p, const toml::table &t) {
        for (const auto &[k, v] : t) {
            if (v.is_table()) {
                visit(p + k + sep, *v.as_table());
            } else if (v.is_string()) {
                map[p + k] = *v.value<std::string>();
            } else {
                WARN(
                    "Ignoring {}, not a string or a table",
                    p + k);
            }
        }
    };

    visit("", tbl);
    return map;
}

extern template std::unordered_map<std::string, std::string> as_map(
    const toml::table &tbl,
    std::unordered_map<std::string, std::string> &&init,
    const std::string &sep);
}
