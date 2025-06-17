#pragma once

#include "util/util.hpp"
#include "util/types.hpp"

struct InitRegistry {
    using InitFn = std::function<void(void)>;

    static InitRegistry &instance();

    void add(InitFn &&f) {
        this->fs.emplace_back(std::move(f));
    }

    void init() {
        for (auto &f : this->fs) {
            f();
        }
    }

private:
    std::vector<InitFn> fs;
};

#define ON_INIT(_f)                                                            \
    static const auto CONCAT(_on_init_, __COUNTER__) =                         \
        (InitRegistry::instance().add((_f)), 0);
