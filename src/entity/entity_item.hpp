#pragma once

#include "entity/entity.hpp"
#include "item/item_stack.hpp"

struct Item;
struct DropTable;

struct EntityItem : public Entity {
    using Base = Entity;

    EntityItem() = default;

    explicit EntityItem(ItemStack &&stack)
        : Base(),
          _stack(std::move(stack)) {
        this->drag = 1.1f;
        this->restitution = 1.05f;
    }

    inline ItemStack &stack() const {
        return this->_stack;
    }

    inline const Item &item() const {
        return this->stack().item();
    }

    void tick() override;

    void render(RenderContext &ctx) override;

    std::string name() const override {
        return "item";
    }

    std::string locale_name() const override;

    std::optional<AABB> aabb() const override;

    LightArray lights() const override;

    bool does_collide(const Entity &other) const override {
        return true;
    }

    bool does_stop(const Entity &other) const override {
        return false;
    }

    void on_collision(Entity &other) override;

    static void drop_from_entity(
        const Entity &entity,
        ItemStack &&stack);

    static void drop(
        Level &level,
        const vec3 &pos,
        const DropTable &drops);

private:
    usize ticks_to_pickup = 80;
    mutable ItemStack _stack;
};
