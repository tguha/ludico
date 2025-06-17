#include "debug.hpp"
#include "input/controls.hpp"

void Debug::tick() {
    for (auto *t : this->visualizers()) {
        if (!t->toggle.empty()
                && Controls::from_string(t->toggle).is_pressed_tick()) {
            t->enabled = !t->enabled;
        }
    }
}
