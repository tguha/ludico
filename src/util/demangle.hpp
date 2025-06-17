#pragma once

#ifdef __GNUG__
#include <string>
#include <memory>
#include <cxxabi.h>

namespace util {
inline std::string demangle(const char *name) {
    int status;

    std::unique_ptr<char, void(*)(void*)> res {
        abi::__cxa_demangle(name, NULL, NULL, &status),
        std::free
    };

    return !status ? res.get() : name;
}
}

#else

namespace util {
inline std::string demangle(const char *name) {
    return name;
}
}

#endif