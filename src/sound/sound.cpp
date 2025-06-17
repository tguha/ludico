#include "sound/sound.hpp"
#include "util/assert.hpp"
#include "util/file.hpp"

#include <soloud.h>
#include <soloud_wav.h>

SoundEngine::SoundEngine() {
    LOG("Initializing sound engine...");
    const auto result =
        this->soloud.init(
            SoLoud::Soloud::CLIP_ROUNDOFF);

    if (result != SoLoud::SO_NO_ERROR) {
        ERROR(
            "Error initializing sound engine: {}",
            this->soloud.getErrorString(result));
    }

    LOG("  backend: {}", soloud.getBackendString());
    LOG("  channels: {}", soloud.getBackendChannels());
    LOG("  sample rate: {}", soloud.getBackendSamplerate());
}

SoundEngine::~SoundEngine() {
    this->soloud.stopAll();
    this->soloud.deinit();
}

void SoundEngine::stop_all() {
    return this->soloud.stopAll();
}

Sound::Sound(SoundEngine &engine, const std::string &path)
    : engine(&engine) {
    LOG("Loading sound from {}", path);

    // TODO: fail more gracefully
    this->wav = std::make_unique<SoLoud::Wav>();
    ASSERT(file::exists(path), "file {} does not exist", path);
    const auto result = this->wav->load(path.c_str());
    ASSERT(result == SoLoud::SO_NO_ERROR);
}

Sound::Handle Sound::play(
    f32 volume,
    const vec3 &pos,
    f32 speed) const {
    ASSERT(this->engine);

    const auto handle =
        this->engine->soloud.play3d(
            *this->wav,
            pos.x,
            pos.y,
            pos.z,
            0.0f,
            0.0f,
            0.0f,
            volume);
    this->engine->soloud.setRelativePlaySpeed(
        handle, speed);
    return handle;
}

void Sound::stop(Handle handle) {
    this->engine->soloud.stop(handle);
}
