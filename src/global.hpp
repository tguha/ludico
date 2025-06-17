#pragma once

#include "util/types.hpp"
#include "util/util.hpp"
#include "util/bump_allocator.hpp"
#include "util/basic_allocator.hpp"
#include "util/time.hpp"
#include "debug.hpp"

struct Controls;
struct Renderer;
struct Platform;
struct Tiles;
struct Items;
struct UIRoot;
struct SoundEngine;
struct State;
struct StateGame;

namespace toml {
    inline namespace v2 {
        class table;
    }
}

struct Global {
    Controls *controls;
    Renderer *renderer;
    Time *time;
    Platform *platform;
    UIRoot *ui;
    SoundEngine *sound;

    // debug controller
    Debug debug;

    // settings, see defaults.toml
    const toml::v2::table *settings;

    // base memory allocator, program lifetime
    BasicAllocator allocator;

    // bump allocator cleared every frame
    BumpAllocator frame_allocator;

    // bump allocator cleared every tick
    BumpAllocator tick_allocator;

    // cleared after 2 frames with no use, use for bgfx calls which require
    // memory
    LongAllocator<2> long_allocator;

    // current game state
    State *state;

    // possible game states
    StateGame *game;

    void tick();
    void update();
};

// global global, see main.cpp
extern Global global;
