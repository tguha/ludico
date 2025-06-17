#pragma once

#include "util/assert.hpp"
#include "util/types.hpp"
#include "util/util.hpp"
#include "util/math.hpp"

#include <soloud.h>
#include <soloud_wav.h>

struct Sound;

struct SoundEngine {
    SoundEngine();
    ~SoundEngine();

    SoundEngine(const SoundEngine&) = delete;
    SoundEngine(SoundEngine&&) = default;
    SoundEngine &operator=(const SoundEngine&) = delete;
    SoundEngine &operator=(SoundEngine&&) = default;

    void set_volume(f32 v) {
        ASSERT(v >= 0.0f && v <= 1.0f);
        this->_volume = v;
        this->soloud.setGlobalVolume(this->_volume);
    }

    auto volume() const {
        return this->_volume;
    }

    // stop all sounds from playing
    void stop_all();

private:
    SoLoud::Soloud soloud;
    f32 _volume;

    friend struct Sound;
};

struct Sound {
    // play "instance" handle
    using Handle = int;

    Sound(SoundEngine &engine, const std::string &path);

    Sound() = default;
    Sound(const Sound&) = delete;
    Sound(Sound&&) = default;
    Sound &operator=(const Sound&) = delete;
    Sound &operator=(Sound&&) = default;

    Handle play(
        f32 volume = 1.0f,
        const vec3 &pos = vec3(0.0f),
        f32 speed = 1.0f) const;

    void stop(Handle handle);

private:
    SoundEngine *engine;
    std::unique_ptr<SoLoud::Wav> wav;

    friend struct SoundEngine;
};
