#include "entity/interaction.hpp"
#include "serialize/serialize.hpp"
#include "global.hpp"

SERIALIZE_ENABLE()
DECL_PRIMITIVE_SERIALIZER_OPT(InteractionKind)
DECL_SERIALIZER_OPT(Interaction)

Interaction::Interaction(
    InteractionKind kind,
    const vec3 &pos,
    ItemRef stack,
    EntityRef source,
    EntityRef target,
    std::optional<ToolKind::Enum> tool)
    : ticks(global.time->ticks),
      kind(kind),
      pos(pos),
      stack(stack),
      source(source),
      target(target),
      tool(tool) {}

Interaction::Interaction(
    InteractionKind kind,
    const vec3 &pos,
    ItemRef stack,
    EntityRef source,
    const std::optional<ivec3> &tile,
    const std::optional<Direction::Enum> &face,
    std::optional<ToolKind::Enum> tool)
    : ticks(global.time->ticks),
      kind(kind),
      pos(pos),
      stack(stack),
      tile(tile),
      face(face),
      source(source),
      tool(tool) {}

bool Interaction::same(const Interaction &other) const {
    return this->kind == other.kind
        && this->stack == other.stack
        && this->source == other.source
        && this->target == other.target
        && this->tile == other.tile
        && this->face == other.face;
}

std::string Interaction::to_string() const {
    return fmt::format(
        "Interaction({}, {}, {}, {}, {}, {}, {}, {})",
        this->kind,
        this->pos,
        this->stack,
        this->tile,
        this->face,
        this->source,
        this->target,
        this->ticks);
}
