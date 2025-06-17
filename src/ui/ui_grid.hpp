#pragma once

#include "ui/ui_container.hpp"

template <typename T>
    requires
        (std::is_default_constructible_v<T>
            && (requires (T t) { t.configure(true); }))
struct UIGrid : virtual public UIContainer {
    using Base = UIContainer;

    std::vector<T*> elements;

    uvec2 grid_size = uvec2(0);

    UIGrid(const uvec2 &grid_size = uvec2(0))
        : UIContainer(),
          grid_size(grid_size) {}

    inline usize num_elements() const {
        return this->grid_size.x * this->grid_size.y;
    }

    inline T &element(const uvec2 &index) {
        return *this->elements[index.y * this->grid_size.x + index.x];
    }

    // (re)create grid
    void make_grid(std::optional<uvec2> new_size = std::nullopt) {
        if (new_size) {
            this->grid_size = *new_size;
        }

        // cannot create empty grid
        if (this->grid_size == uvec2(0)) {
            return;
        }

        LOG("making a grid with size {}", this->grid_size);

        this->elements.clear();
        this->elements.resize(this->num_elements());

        this->clear();

        for (usize y = 0; y < this->grid_size.y; y++) {
            for (usize x = 0; x < this->grid_size.x; x++) {
                const auto p = uvec2(x, y);

                auto &el = this->add(std::make_unique<T>());
                el.configure(true);
                this->elements[y * this->grid_size.x + x] = &el;
                this->create_element(el, p);
                this->reposition_element(el, p);
            }
        }

        LOG("SIZE IS {}", this->size);
        this->reposition();
    }

    UIGrid &configure(const uvec2 &size, bool enabled = true) {
        Base::configure(
            size,
            enabled);
        this->make_grid();
        return *this;
    }

    void reposition() override {
        Base::reposition();

        for (usize y = 0; y < this->grid_size.y; y++) {
            for (usize x = 0; x < this->grid_size.x; x++) {
                this->reposition_element(this->element({ x, y }), { x, y });
            }
        }
    }

    // create an element
    virtual void create_element(T &element, const uvec2 &index) = 0;

    // reposition a element
    virtual void reposition_element(T &element, const uvec2 &index) = 0;
};
