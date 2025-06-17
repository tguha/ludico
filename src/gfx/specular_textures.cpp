#include "gfx/specular_textures.hpp"
#include "global.hpp"

const SpecularTextures &SpecularTextures::instance() {
    static SpecularTextures instance;
    return instance;
}

SpecularTextures::SpecularTextures() {
    constexpr auto SIZE = 16;
    std::array<u32, SIZE * SIZE> data;

    for (usize i = 0; i < COUNT; i++) {
        const auto v = math::clamp<u32>((256 / (COUNT - 1)) * i, 0, 255);
        std::fill(
            data.begin(),
            data.end(),
            (0xFF << 24)
            | (v << 16)
            | (v << 8)
            | (v << 0));
        this->areas[i] =
            TextureAtlas::get().add(
                fmt::format("specular_{}", i),
                reinterpret_cast<const u8*>(&data[0]),
                uvec2(SIZE),
                uvec2(SIZE)).to_area();
    }
}
