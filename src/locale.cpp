#include "locale.hpp"
#include "util/toml.hpp"

Locale &Locale::instance() {
    static Locale instance;
    return instance;
}

Locale Locale::from_toml(const std::string &path) {
    LOG("Loading locale from {}", path);
    return Locale(
        toml::as_map(
            toml::parse(file::read_string(path).unwrap()),
            std::unordered_map<std::string, std::string>(),
            ":"));
}
