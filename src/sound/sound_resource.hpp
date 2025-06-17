#pragma once

#include "sound/sound.hpp"
#include "util/managed_resource.hpp"
#include "platform/platform.hpp"
#include "global.hpp"

struct SoundResource
    : public ValueManagedResource<Sound> {
    using Base = ValueManagedResource<Sound>;
    using SourceFn = std::function<Sound(void)>();

    using Base::Base;

    explicit SoundResource(const std::string &path)
        : Base(
            [=]() {
                return Sound(
                    *global.sound,
                    global.platform->resources_path + "/" + path);
            }) {}

    void attach() override {}
};
