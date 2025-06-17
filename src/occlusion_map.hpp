#pragma once

#include "gfx/texture.hpp"
#include "level/chunk.hpp"

struct GameCamera;

struct OcclusionMap {
    static constexpr auto SIZE = uvec3(48, Chunk::SIZE.y, 48);

    std::vector<u8> data_full, data_top, data_blocking;
    Texture texture_full, texture_top, texture_blocking;

    struct BlockingInfo {
        usize first, last;
    };

    // list of blocking tiles including the tick on which they were inserted
    // NOTE: stored in world space (not occlision map space!)
    std::unordered_map<ivec3, BlockingInfo> blocking;

    // offset of data origin in world space
    ivec3 offset;

    OcclusionMap() = default;

    void init();

    void update(
        const GameCamera &camera,
        Level &level,
        const ivec2 &center);

    void set_uniforms(
        const Program &program,
        u8 sampler_stage_full,
        u8 sampler_stage_top,
        u8 sampler_stage_blocking) const;
};
