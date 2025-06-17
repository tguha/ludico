#include "util/toml.hpp"

namespace toml {
template std::unordered_map<std::string, std::string> as_map(
    const toml::table &tbl,
    std::unordered_map<std::string, std::string> &&init,
    const std::string &sep);
}
