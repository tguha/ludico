#pragma once

#include "util/types.hpp"
#include "util/math.hpp"
#include "gfx/bgfx.hpp"

struct Cursor;

struct Window {
    virtual ~Window() = default;
    virtual void set_platform_data(
            bgfx::PlatformData &platform_data
        ) = 0;
    virtual std::optional<Cursor*> cursor() = 0;
    virtual void prepare_frame() = 0;
    virtual void end_frame() = 0;
    virtual bool is_close_requested() = 0;
    virtual void close() = 0;
    virtual bool resized() = 0;
    virtual uvec2 size() = 0;
};
