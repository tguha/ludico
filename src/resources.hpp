#pragma once

#include "util/util.hpp"

struct Renderer;
struct TextureAtlas;

namespace resources {
void load(
    const std::string &base_path,
    Renderer &renderer);
}
