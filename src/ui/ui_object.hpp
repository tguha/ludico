#pragma once

#include "util/math.hpp"
#include "util/util.hpp"
#include "util/types.hpp"
#include "util/hash.hpp"
#include "util/rand.hpp"
#include "gfx/util.hpp"
#include "gfx/sprite_batch.hpp"
#include "gfx/quad_batch.hpp"

struct UIRoot;

struct UIObject {
    enum Align {
        ALIGN_LEFT      = (1 << 0),
        ALIGN_RIGHT     = (1 << 1),
        ALIGN_TOP       = (1 << 2),
        ALIGN_BOTTOM    = (1 << 3),
        ALIGN_CENTER_H  = (1 << 4),
        ALIGN_CENTER_V  = (1 << 5)
    };

    // global ui root, never nullptr
    UIRoot *root;

    // parent of this object, possibly nullptr
    UIObject *parent;

    ivec2 pos = ivec2(0);
    uvec2 size = uvec2(0);
    u64 id = 0;

    // z-index for ordering under rendering
    isize z = 0;

    UIObject();
    UIObject(const UIObject &other) = delete;
    UIObject(UIObject &&other) = default;
    UIObject &operator=(const UIObject &other) = delete;
    UIObject &operator=(UIObject &&other) = default;
    virtual ~UIObject() = default;

    // trigger for addition to container
    virtual void on_add() {}

    UIObject &configure(const uvec2 &size, bool enabled = true) {
        this->size = size;
        this->set_enabled(enabled);
        return *this;
    }

    UIObject &align(
        usize alignment,
        UIObject *relative_to = nullptr);

    // detach this UI object from its parent
    void detach();

    virtual void update() {}

    virtual void tick() {}

    virtual void render(SpriteBatch &batch, RenderState render_state) {}

    virtual void render_debug(
        SpriteBatch &batch_s,
        QuadBatch &batch_q,
        RenderState render_state) {
        auto rand = Rand(hash(this->id));

        const auto alpha = this->is_hovered() ? 0.8f : 0.5f;
        batch_q.push_outline(
            this->pos_absolute(),
            this->size,
            vec4(rand.next(vec3(0.0f), vec3(1.0f)), alpha));
    }

    // reposition (on resize, for example)
    virtual void reposition() {}

    // true if this UI object should ignore the cursor and only paint on the
    // screen (it is non-interactable)
    // DOES NOT mean that its children will also ignore such things, ghost
    // containers will still pass events down
    virtual bool is_ghost() const { return false; }

    // attempt to click this object, pos relative
    virtual bool click(const ivec2 &pos, bool is_release) { return false; }

    // attempt hover over this object, pos relative
    virtual bool hover(const ivec2 &pos) { return true; }

    // returns true if this ui object is currently being hovered over
    bool is_hovered() const;

    // returns true if this ui object is currently being clicked on
    bool is_pressed() const;

    // absolute (screen) position
    virtual ivec2 pos_absolute() const {
        return this->parent ?
            (this->parent->pos_absolute() + this->pos)
            : this->pos;
    }

    // absolute aabb
    virtual AABB2i aabb_absolute() const {
        const auto pos_abs = this->pos_absolute();
        return AABB2i(pos_abs, pos_abs + ivec2(this->size) - 1);
    }

    // relative aabb (RELATIVE TO PARENT!)
    virtual AABB2i aabb() const {
        return AABB2i(this->pos, this->pos + ivec2(this->size) - 1);
    }

    inline std::string to_string() const {
        return fmt::format(
            "UIObject(id={},pos={},size={})",
            this->id, this->pos, this->size);
    }

    virtual void set_enabled(bool e) {
        const auto old = this->_enabled;
        this->_enabled = e;

        if (this->_enabled != old) {
            this->on_enabled_changed();
        }
    }

    inline bool is_enabled() const {
        return this->_enabled;
    }

    virtual void on_enabled_changed() {}

private:
    bool _enabled = false;
};
