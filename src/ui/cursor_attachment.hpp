#pragma once

#include "gfx/util.hpp"
#include "gfx/sprite_batch.hpp"

struct UIObject;

// object attached to cursor, managed by UI root
struct CursorAttachment {
    virtual ~CursorAttachment() = default;

    // render relative to cursor tip
    virtual void render(
        const ivec2 &cursor, SpriteBatch &batch, RenderState render_state) {}

    virtual void tick(const ivec2 &cursor) {}

    // called when click occurs with this cursor attachment
    virtual void on_click(bool left, UIObject *obj) {}

    // remove this cursor attachment
    // NOTE: returns the unique_ptr to ensure that CursorAttachments which call
    // remove on themselves aren't invalidated while running (they live on with
    // the unique_ptr).
    std::unique_ptr<CursorAttachment> remove();
};
