#pragma once

#include "entity/entity_ref.hpp"
#include "ui/ui_event.hpp"

struct UIEventPlayerChange : public UIEvent {
    EntityRef player;

    UIEventPlayerChange() = default;

    explicit UIEventPlayerChange(EntityRef player)
        : player(player) {}
};
