#include "entity/entity_dandelion.hpp"
#include "model/model_dandelion.hpp"
#include "gfx/renderer_resource.hpp"
#include "gfx/render_context.hpp"
#include "item/item_seed_dandelion.hpp"
#include "serialize/serialize.hpp"

SERIALIZE_ENABLE()
DECL_SERIALIZER(EntityDandelion)

static auto model_manager =
    RendererResource<ModelDandelion>(
        []() { return std::make_unique<ModelDandelion>(); });

EntityDandelion::EntityDandelion()
    : Entity(),
      EntityModeled(),
      EntityColored(model_manager->colors()) {}

void EntityDandelion::render(RenderContext &ctx) {
    ctx.push_group(RenderContext::GROUP_ID_DEFAULT_INSTANCE);
    {
        model_manager->render(
            ctx,
            *this);
    }
    ctx.pop_group();

}

const ModelDandelion &EntityDandelion::model() const {
    return model_manager.get();
}

EntityPlant::StageDrops EntityDandelion::stage_drops() const {
    using P = EntityPlant::StageDrops::value_type;

    const auto &item_seed = Items::get().get<ItemSeedDandelion>();
    return {
        P(STAGE_0, { item_seed }),
        P(STAGE_1, { item_seed }),
        P(STAGE_2, { item_seed }),
        P(STAGE_3, { item_seed }),
        P(STAGE_4, { item_seed }),
        P(STAGE_5, { item_seed })
    };
}
