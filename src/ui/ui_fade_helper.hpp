#pragma once

#include "util/time.hpp"
#include "util/types.hpp"
#include "global.hpp"

template <typename T>
struct UIFadeHelper {
    usize fade_ticks = 0, hold_ticks = 0;

    explicit UIFadeHelper(usize fade_ticks, T &&t_init = T())
        : fade_ticks(fade_ticks),
          t(t_init) {}

    bool is_set_this_tick() const {
        return this->last_set_ticks == global.time->ticks;
    }

    void set(const T &t = T(), usize hold_ticks = 0) {
        this->last_set_ticks = global.time->ticks;
        this->hold_ticks = hold_ticks;
        this->t = t;
    }

    bool done() const {
        return (global.time->ticks - this->last_set_ticks)
            > this->fade_ticks + this->hold_ticks;
    }

    f32 value() const {
        return
            this->hold_ticks >= global.time->ticks - this->last_set_ticks ?
                1.0 :
                (1.0 - (f32(global.time->ticks - (this->last_set_ticks + this->hold_ticks))
                    / this->fade_ticks));
    }

    const T &last() const {
        return this->t;
    }

private:
    T t;
    usize last_set_ticks = 3918739857;
};
