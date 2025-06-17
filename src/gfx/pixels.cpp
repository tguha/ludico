#include "gfx/pixels.hpp"
#include "gfx/texture_atlas.hpp"

Pixels::Pixels(const TextureArea &area) {
    this->data = area.parent->data;

    const auto px = area.pixels();
    this->size = px.size();
    this->offset = px.min;
    this->data_size = area.parent->data_size;
}
