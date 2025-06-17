#include "global.hpp"
#include "util/time.hpp"
#include "platform/platform.hpp"
#include "ui/ui_root.hpp"

void Global::tick() {
    this->platform->tick();
    this->ui->tick();
    this->debug.tick();
}

void Global::update() {
    this->platform->update();
    this->ui->update();
}
