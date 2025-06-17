#pragma once

#include "gfx/util.hpp"

// game state
struct State {
    enum Type {
        STATE_MAIN_MENU,
        STATE_GAME
    };

    Type type;

    State() = delete;
    State(Type type) : type(type) {}

    virtual ~State() = default;

    State(const State&) = delete;
    State(State&&) = default;
    State &operator=(const State&) = delete;
    State &operator=(State&&) = delete;

    virtual void init() {}
    virtual void destroy() {}
    virtual void tick() {}
    virtual void update() {}
    virtual void render() {}
};
