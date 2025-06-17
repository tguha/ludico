#include "init_registry.hpp"

InitRegistry &InitRegistry::instance() {
    static InitRegistry instance;
    return instance;
}
