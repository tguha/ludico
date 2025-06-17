#pragma once

#include "util/util.hpp"
#include "util/log.hpp"

template <typename T, typename U>
struct TypeRegistry {
    std::vector<std::function<void(T&)>> fs;

    // must be declared in some translation unit
    static TypeRegistry<T, U> &instance();

    template <typename V>
    void add() {
        fs.push_back([](T &t){
            LOG("Registering {}", typeid(V).name());
            t.template register_type<V>();
        });
    }

    void initialize(T &t) {
        for (const auto &f : this->fs) {
            f(t);
        }
    }
};

template <typename R, typename T>
struct TypeRegistrar {
    TypeRegistrar() {
        R::instance().template add<T>();
    }
};
