#include "platform/platform.hpp"
#include "platform/window.hpp"
#include "input/input.hpp"

Platform::Platform() = default;

Platform::~Platform() {
    bgfx::shutdown();
}

void Platform::update() {
    for (auto &[_, input] : this->inputs) {
        input->update();
    }
}

void Platform::tick() {
    for (auto &[_, input] : this->inputs) {
        input->tick();
    }
}
