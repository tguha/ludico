#pragma once

#include "util/types.hpp"
#include "util/util.hpp"
#include "util/math.hpp"
#include "global.hpp"

template <typename V>
struct Keyframe {
    static constexpr auto TICK_NONE =
        std::numeric_limits<u64>::max();

    // tick at which animate() == value
    u64 tick = TICK_NONE;

    // target value
    V target;

    // keyframe previous to this frame
    std::optional<std::tuple<u64, V>> prev;

    Keyframe() = default;
    Keyframe(
        u64 tick,
        const V &target,
        std::optional<Keyframe<V>> prev = std::nullopt)
        : tick(tick),
          target(target),
          prev(
              prev ?
                std::make_optional(std::make_tuple(prev->tick, prev->target))
                : std::nullopt) {}

    inline operator std::tuple<u64, V>() const {
        return std::make_tuple(this->tick, this->target);
    }

    inline bool has_value() const {
        return this->tick != TICK_NONE;
    }

    // get status of this keyframe relative to previous
    inline V get(
        std::optional<Keyframe<V>> prev = std::nullopt,
        u64 current = global.time->ticks) const {
        prev = prev ?
            prev
            : (this->prev ?
                std::make_optional(
                    Keyframe<V>(
                        std::get<0>(*this->prev),
                        std::get<1>(*this->prev)))
                : std::nullopt);

        if (!prev) {
            return this->target;
        }

        return math::lerp(
            prev->target,
            this->target,
            math::clamp(
                (current - prev->tick)
                    / static_cast<f32>(this->tick - prev->tick),
                0.0f,
                1.0f));
    }

    // true if keyframe has been reached
    inline bool done() const {
        return !this->has_value() || global.time->ticks >= this->tick;
    }

    // returns *this if not done, otherwith k
    inline Keyframe<V> and_then(Keyframe<V> &&k) const {
        return this->done() ? std::move(k) : *this;
    }
};

template <typename V>
struct KeyframeList {
    std::vector<Keyframe<V>> keyframes;

    KeyframeList() : KeyframeList({}) {}
    KeyframeList(
        std::initializer_list<std::pair<u64, V>> &&values,
        std::optional<Keyframe<V>> prev = std::nullopt,
        u64 start = global.time->ticks) {
        u64 acc = start;
        std::transform(
            values.begin(),
            values.end(),
            std::back_inserter(this->keyframes),
            [&](const auto &p) {
                const auto [time, value] = p;
                acc += time;
                return Keyframe<V>(acc, value);
            });

        if (this->keyframes.empty()) {
            this->keyframes.push_back(Keyframe<V>());
        }

        // set up previous frames
        this->keyframes[0].prev = prev;
        for (usize i = 1; i < this->keyframes.size(); i++) {
            this->keyframes[i].prev = this->keyframes[i - 1];
        }
    }

    inline bool has_value() const {
        return this->last().has_value();
    }

    inline const Keyframe<V> &current(u64 time = global.time->ticks) const {
        if (this->keyframes.empty()) {
            const_cast<KeyframeList<V>*>(this)->keyframes =
                { Keyframe<V>() };
        }

        auto it = std::find_if(
            this->keyframes.begin(),
            this->keyframes.end(),
            [&](const auto &k) {
                return k.tick >= time;
            });

        return it == this->keyframes.end() ? this->last() : *it;
    }

    inline Keyframe<V> as_keyframe(u64 time = global.time->ticks) const {
        return Keyframe<V>(time, this->get());
    }

    inline const Keyframe<V> &last() const {
        return this->keyframes.at(this->keyframes.size() - 1);
    }

    inline bool done() const {
        return this->last().done();
    }

    inline V get() const {
        return this->current().get();
    }
};
