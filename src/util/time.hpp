#pragma once

#include "util/util.hpp"
#include "util/log.hpp"
#include "util/types.hpp"
#include "constants.hpp"

struct Time {
    using NowFn = std::function<u64(void)>;
    using TickFn = std::function<void(void)>;

    static constexpr u64
        NANOS_PER_SECOND    = 1000000000,
        NANOS_PER_MILLIS    = 1000000,
        MILLIS_PER_SECOND   = 1000,
        NANOS_PER_TICK      = (NANOS_PER_SECOND / TICKS_PER_SECOND),
        TICK_MAX            = std::numeric_limits<u64>::max();

    template <typename T>
    static inline auto to_millis(T nanos) {
        return nanos / static_cast<T>(NANOS_PER_MILLIS);
    }

    template <typename T>
    static inline auto to_seconds(T nanos) {
        return nanos / static_cast<T>(NANOS_PER_SECOND);
    }

    struct Section {
        static constexpr usize RUNNING_AVG_LENGTH = 120;

        std::array<u64, RUNNING_AVG_LENGTH> times;
        u64 start;

        Section() = default;
        Section(Time *time, bool use_ticks = false)
            : time(time), use_ticks(use_ticks) {
            memset(&this->times, 0, sizeof(this->times));
        }

        inline void begin() {
            this->start = this->time->now();
        }

        inline void end() {
            this->times[
                (this->use_ticks ? this->time->ticks : this->time->frames)
                    % RUNNING_AVG_LENGTH] =
                this->time->now() - this->start;
        }

        inline f64 avg() {
            u64 sum = 0;
            for (auto i : this->times) {
                sum += i;
            }
            return sum / static_cast<f64>(RUNNING_AVG_LENGTH);
        }

    private:
        Time *time;
        bool use_ticks;
    };

    Section
        section_update,
        section_render,
        section_tick,
        section_frame;

    u64 time, last_frame, last_second;
    u64 ticks, second_ticks, tps, tick_remainder, frame_ticks;
    u64 frames, second_frames, fps;
    u64 delta;

    NowFn now;

    Time() = default;
    Time(const Time &other) = delete;
    Time(Time &&other) = default;
    Time &operator=(const Time &other) = delete;
    Time &operator=(Time &&other) = default;

    explicit Time(NowFn &&now) {
        std::memset(this, 0, sizeof(*this));
        this->now = std::move(now);
        this->section_update = Section(this);
        this->section_render = Section(this);
        this->section_tick = Section(this, true);
        this->section_frame = Section(this);
        this->time = this->now();
        this->last_frame = this->now();
        this->last_second = this->now();
    }

    void tick(TickFn &&tick);
    void update();

    template <typename F>
        requires std::is_invocable_v<F>
    inline void time_section(const std::string &name, F &&f) {
        const auto start = this->now();
        f();
        LOG(
            "section {}: {:.3} ms", name, to_millis<f64>(this->now() - start));
    }
};
