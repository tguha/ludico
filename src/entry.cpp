#include "entry.hpp"

EntryManager &EntryManager::instance() {
    static EntryManager instance;
    return instance;
}
