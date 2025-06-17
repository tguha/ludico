#pragma once

#include "util/direction.hpp"
#include "util/math.hpp"
#include "entity/entity_ref.hpp"
#include "item/item_ref.hpp"
#include "item/tool_kind.hpp"

enum class InteractionKind : u8 {
    PRIMARY,
    SECONDARY
};

// contains an entity-entity or entity-tile interaction
struct Interaction {
    usize ticks;

    InteractionKind kind;
    vec3 pos;
    ItemRef stack;
    std::optional<ivec3> tile;
    std::optional<Direction::Enum> face;
    EntityRef source, target;
    std::optional<ToolKind::Enum> tool;

    Interaction() = default;

    Interaction(
        InteractionKind kind,
        const vec3 &pos,
        ItemRef stack,
        EntityRef source,
        EntityRef target,
        std::optional<ToolKind::Enum> tool = std::nullopt);

    Interaction(
        InteractionKind kind,
        const vec3 &pos,
        ItemRef stack,
        EntityRef source,
        const std::optional<ivec3> &tile,
        const std::optional<Direction::Enum> &face,
        std::optional<ToolKind::Enum> tool = std::nullopt);

    bool same(const Interaction &other) const;

    std::string to_string() const;
};

// entities (E), items (I), and tiles (T) can all express if an interaction is
// "afforded" (allowed) through InteractionAffordance
//
// OVERRIDE indicates that the action with be performed for E/I/T regardless of
// anything else (ignores NO)
//
// YES indicates that the interaction can be performed for any of E/I/T if the
// others of E/I/T respond with either YES, NONE, or OVERRIDE
//
// NO indicates that the action cannot be performed for E/I/T, and no
// interact(...) will be called for any of E/I/T
//
// NONE indicates that the action is not explicitly allowed, but it will not
// be stopped (NO)
//
// the interact(...) function is called if the affordance is YES and an
// interaction is performed
//
// "charge" is the cost required to perform the action, should it be YES
struct InteractionAffordance {
    enum Type {
        OVERRIDE,
        YES,
        NO,
        NONE
    };

    Type type = NONE;
    f32 charge = 1.0f;
    ToolKind::Enum tool = ToolKind::HAMMER;

    InteractionAffordance() = default;

    InteractionAffordance(
        Type type,
        f32 charge = 1.0f,
        ToolKind::Enum tool = ToolKind::HAMMER)
        : type(type),
          charge(charge),
          tool(tool) {}

    // "positive"/should interact(...) be called
    bool positive() const {
        return this->type == InteractionAffordance::YES
            || this->type == InteractionAffordance::OVERRIDE;
    }

    bool operator==(const InteractionAffordance &rhs) const = default;

    std::string to_string() const {
        constexpr const char *LOOKUP[] = { "OVERRIDE", "YES", "NO", "NONE" };
        return fmt::format(
            "InteractionAffordance({}, {})",
            std::string_view(LOOKUP[this->type]),
            this->charge);
    }

    // combines interactions, returning (interactions, total charge)
    // returns nullopt if all are NO or NONE (nothing needs to be called/there
    // is no action to perform)
    template <usize N>
    static inline std::optional<
        std::tuple<std::array<InteractionAffordance, N>, f32>> combine(
        const std::array<InteractionAffordance, N> &as) {
        auto result = as;
        // if any are no, zero all non-OVERRIDE
        if (std::any_of(
                result.begin(),
                result.end(),
                [](const auto &a) {
                    return a.type == InteractionAffordance::NO;
                })) {
            for (auto &a : result) {
                if (a.type != InteractionAffordance::OVERRIDE) {
                    a = InteractionAffordance::NONE;
                }
            }
        }

        // nothing to do if all are NO or NONE
        if (std::all_of(
                result.begin(),
                result.end(),
                [](const auto &a) {
                    return a.type == InteractionAffordance::NO
                        || a.type == InteractionAffordance::NONE;
                })) {
            return std::nullopt;
        }

        // compute total cost
        f32 charge = 1.0f;
        for (auto &a : result) {
            if (a.type != InteractionAffordance::NO) {
                charge *= a.charge;
            }
        }

        return std::make_tuple(result, charge);
    }

    // find the tool of the first positive interaction affordance
    template <usize N>
    static inline std::optional<ToolKind::Enum> get_tool(
        const std::array<InteractionAffordance, N> &as) {
        std::optional<ToolKind::Enum> result = std::nullopt;
        for (const auto &a : as) {
            if (a.positive()) {
                result = a.tool;
                break;
            }
        }
        return result;
    }
};

// the result of a performed interaction
struct InteractionResult {
    enum Type {
        // interaction was attempted and succeeded
        SUCCESS,

        // interaction was attempted and failed
        FAILURE,

        // interaction was not attempted
        NONE
    };

    Type type = NONE;

    InteractionResult() = default;
    InteractionResult(Type type)
        : type(type) {}

    bool operator==(const InteractionResult &rhs) const = default;

    std::string to_string() const {
        constexpr const char *LOOKUP[] = { "SUCCESS", "FAILURE", "NONE" };
        return fmt::format(
            "InteractionResult({})",
            std::string_view(LOOKUP[this->type]));
    }
};
