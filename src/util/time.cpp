#include "util/time.hpp"
#include "util/log.hpp"

void Time::tick(TickFn &&tick) {
    for (usize i = 0; i < this->frame_ticks; i++) {
        this->ticks++;
        tick();
    }
}

void Time::update() {
    this->frames++;
    this->second_frames++;

    this->time = this->now();
    this->delta = this->time - this->last_frame;
    this->last_frame = this->time;

    if (this->time - this->last_second >= Time::NANOS_PER_SECOND) {
        this->last_second = this->time;
        this->tps = this->second_ticks;
        this->fps = this->second_frames;
        this->second_ticks = 0;
        this->second_frames = 0;
    }

    auto tick_time = this->tick_remainder + this->delta;
    this->frame_ticks =
        tick_time / Time::NANOS_PER_TICK;
    this->tick_remainder =
        tick_time % Time::NANOS_PER_TICK;
    this->second_ticks += this->frame_ticks;
}
