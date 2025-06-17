#include "gfx/uniform.hpp"
#include "gfx/program.hpp"

void UniformValueList::try_apply(const Program &program) const {
    for (const auto &[n, v] : this->vs) {
        program.try_set(n, v);
    }
}

void UniformValueList::apply(const Program &program) const {
    for (const auto &[n, v] : this->vs) {
        program.set(n, v);
    }
}
