#include "entity/entity_daisy.hpp"
#include "model/model_daisy.hpp"
#include "gfx/renderer_resource.hpp"
#include "gfx/render_context.hpp"
#include "item/item_seed_daisy.hpp"
#include "serialize/serialize.hpp"

SERIALIZE_ENABLE()
DECL_SERIALIZER(EntityDaisy)

static auto model_manager =
    RendererResource<ModelDaisy>(
        []() { return std::make_unique<ModelDaisy>(); });

EntityDaisy::EntityDaisy()
    : Entity(),
      EntityModeled(),
      EntityColored(model_manager->colors()) {}

void EntityDaisy::render(RenderContext &ctx) {
    ctx.push_group(RenderContext::GROUP_ID_DEFAULT_INSTANCE);
    {
        model_manager->render(
            ctx,
            *this);
    }
    ctx.pop_group();
}

const ModelDaisy &EntityDaisy::model() const {
    return model_manager.get();
}

EntityPlant::StageDrops EntityDaisy::stage_drops() const {
    using P = EntityPlant::StageDrops::value_type;

    const auto &item_seed = Items::get().get<ItemSeedDaisy>();
    return {
        P(STAGE_0, { item_seed }),
        P(STAGE_1, { item_seed }),
        P(STAGE_2, { item_seed }),
        P(STAGE_3, { item_seed }),
        P(STAGE_4, { item_seed }),
    };
}
