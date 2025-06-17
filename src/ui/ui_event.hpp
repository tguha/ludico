#pragma once

#include "util/types.hpp"
#include "util/util.hpp"
#include "ui/ui_container.hpp"

struct UIEvent {};

template <typename E>
    requires std::is_base_of_v<UIEvent, E>
struct UIEventHandler {
    virtual ~UIEventHandler() = default;

    virtual void on_event(const E &event) {
        if (const auto *container =
                dynamic_cast<UIContainer*>(this)) {
            for (const auto &child : *container) {
                if (auto *obj =
                        dynamic_cast<UIEventHandler<E>*>(child.get())) {
                    obj->on_event(event);
                }
            }
        }
    }
};
