#include "entity/entity_sunflower.hpp"
#include "model/model_sunflower.hpp"
#include "gfx/renderer_resource.hpp"
#include "gfx/render_context.hpp"
#include "item/item_seed_sunflower.hpp"
#include "serialize/serialize.hpp"

SERIALIZE_ENABLE()
DECL_SERIALIZER(EntitySunflower)

static auto model_manager =
    RendererResource<ModelSunflower>(
        []() { return std::make_unique<ModelSunflower>(); });

EntitySunflower::EntitySunflower()
    : Entity(),
      EntityModeled(),
      EntityColored(model_manager->colors()) {}

void EntitySunflower::render(RenderContext &ctx) {
    ctx.push_group(RenderContext::GROUP_ID_DEFAULT_INSTANCE);
    {
        model_manager->render(
            ctx,
            *this);
    }
    ctx.pop_group();
}

const ModelSunflower &EntitySunflower::model() const {
    return model_manager.get();
}

EntityPlant::StageDrops EntitySunflower::stage_drops() const {
    using P = EntityPlant::StageDrops::value_type;

    const auto &item_seed = Items::get().get<ItemSeedSunflower>();
    return {
        P(STAGE_0, { item_seed }),
        P(STAGE_1, { item_seed }),
        P(STAGE_2, { item_seed }),
        P(STAGE_3, { item_seed }),
        P(STAGE_4, { item_seed }),
    };
}
