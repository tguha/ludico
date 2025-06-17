#pragma once

#include "util/math.hpp"
#include "util/util.hpp"
#include "util/types.hpp"
#include "ui/ui_object.hpp"

struct UIContainer : virtual public UIObject {
    using Base = UIObject;

    std::vector<std::unique_ptr<UIObject>> children;

    // iterators
    auto begin() { return this->children.begin(); }
    auto end() { return this->children.end(); }
    auto begin() const { return this->children.begin(); }
    auto end() const { return this->children.end(); }

    // container functions
    inline auto &operator[](usize i) { return *this->children[i]; }
    inline auto &operator[](usize i) const { return *this->children[i]; }
    inline usize count() const { return this->children.size(); }
    inline bool empty() const { return this->children.empty(); }

    template <typename T>
        requires std::is_base_of_v<UIObject, T>
    T &add(std::unique_ptr<T> &&object) {
        ASSERT(object);
        ASSERT(object->id != 0);


        auto &obj =
                this->children.emplace_back(std::move(object));

        // maintain sorting by id
        std::sort(
            this->children.begin(), this->children.end(),
            [](const auto &a, const auto &b) {
                return a->id < b->id;
            });

        obj->parent = this;
        obj->on_add();

        return dynamic_cast<T&>(*obj);
    }

    UIContainer &clear() {
        this->children.clear();
        return *this;
    }

    UIContainer &remove(UIObject &object);

    bool contains(UIObject &object);

    void update() override;

    void tick() override;

    void render(SpriteBatch &batch, RenderState render_state) override;

    void render_debug(
        SpriteBatch &batch_s,
        QuadBatch &batch_q,
        RenderState render_state) override;

    void reposition() override;

    bool click(const ivec2 &pos, bool is_release) override;

    bool hover(const ivec2 &pos) override;

    // returns topmost object in this container at the specified position
    // nullptr if no such object
    // NOTE: position is relative!
    UIObject *topmost(
        const ivec2 &pos, bool include_ghost = false) const;

    // returns the topmost object in this container AND searches through child
    // containers if present
    // nullptr if no such object
    UIObject *topmost_recursive(
        const ivec2 &pos, bool include_ghost = false) const;
};
