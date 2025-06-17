#include "entity/entity_item.hpp"
#include "entity/entity_render.hpp"
#include "entity/entity_mob.hpp"
#include "entity/entity_sparkle_particle.hpp"
#include "level/level.hpp"
#include "item/item_tile.hpp"
#include "item/item_icon.hpp"
#include "item/drop_table.hpp"
#include "sound/sound_resource.hpp"
#include "serialize/serialize.hpp"
#include "util/hash.hpp"
#include "util/rand.hpp"
#include "gfx/render_context.hpp"
#include "constants.hpp"
#include "global.hpp"

SERIALIZE_ENABLE()
DECL_SERIALIZER(EntityItem)

void EntityItem::tick() {
    Base::tick();

    if (this->ticks_to_pickup > 0) {
        this->ticks_to_pickup--;
    }

    auto rand = Rand(hash(this->id, global.time->ticks));
    if (rand.chance(0.0075)
            && global.game->camera->possibly_visible(*this)) {
        EntitySparkleParticle::spawn_on_entity(*this, vec3(16.0));
    }
}

void EntityItem::render(
    RenderContext &ctx) {
    // TODO: collision box placing
    /* Renderer::get().debug->add( */
    /*     gfx::PrimitiveBox(*this->aabb(), vec4(1.0, 0.0, 0.0, 1.0))); */

    // TODO: fix items to render directly on top of their collision boxes
    auto m = mat4(1.0);
    m = math::translate(m, this->pos);
    m *= math::dir_to_rotation(
            math::xz_to_xyz(
                global.game->camera->rotation_direction(),
                0.0f));

    if (dynamic_cast<const ItemTile*>(&this->item())) {
        m = math::rotate(m, math::PI_4, vec3(0, 1, 0));
    }

    auto &group_old = ctx.top_group();
    auto &group = ctx.push_group();
    {
        group.inherit(group_old);
        group.group_flags &= ~RenderGroup::INSTANCED;
        group.program = &Renderer::get().programs["model"];

        entity_render::configure_group(group, *this);
        this->item().icon().render(
            ctx,
            &this->stack(),
            m);
    }
    ctx.pop_group();
}

std::string EntityItem::locale_name() const {
    return this->stack().item().locale_name();
}

std::optional<AABB> EntityItem::aabb() const {
    return AABB::unit().scale(0.3f).translate(this->pos);
}

LightArray EntityItem::lights() const {
    const auto offset = vec3(0.0f, this->aabb()->size().y, 0.0f);
    auto ls = this->item().lights(this->stack());
    for (auto &l : ls) {
        l.pos = this->center + offset;
    }
    return ls;
}

void EntityItem::on_collision(Entity &other) {
    if (this->should_destroy) {
        return;
    }

    if (this->ticks_to_pickup > 0) {
        return;
    }

    if (auto inventory = other.inventory()) {
        if (inventory->can_add(this->stack())) {
            auto [added, remainder] =
                inventory->try_add(std::move(this->stack()));

            if (added) {
                other.on_pickup(*added, this);
            }

            if (remainder) {
                this->_stack = std::move(*remainder);
            } else {
                this->destroy();
            }
        }
    }
}

void EntityItem::drop_from_entity(
    const Entity &entity,
    ItemStack &&stack) {
    auto entity_item =
        entity.level->spawn<EntityItem>(
            entity.center,
            std::move(stack));

    if (!entity_item) {
        return;
    }

    entity_item->velocity = 0.1f * entity.direction.lerped();
}

void EntityItem::drop(
    Level &level,
    const vec3 &pos,
    const DropTable &drops) {
    auto items = drops.get(level);
    for (auto &&stack : items) {
        auto entity_item = level.spawn<EntityItem>(pos, std::move(stack));

        if (entity_item) {
            auto rand = Rand(hash(entity_item->id));
            entity_item->velocity +=
                0.06f * math::normalize(vec3(
                    rand.next(-1.0f, 1.0f),
                    rand.next(0.5f, 1.6f),
                    rand.next(-1.0f, 1.0f)
                ));
        }
    }
 }
