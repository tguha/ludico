#include "gfx/texture_atlas.hpp"
#include "gfx/bgfx.hpp"
#include "util/time.hpp"
#include "global.hpp"

TextureAtlasEntry::TextureAtlasEntry(
    TextureAtlas &atlas,
    const std::string &label,
    const AABB2u &bounds,
    const uvec2 &offset,
    const uvec2 &size,
    const uvec2 &unit_size)
    : atlas(&atlas),
      label(label),
      bounds(bounds) {
    this->data = this->atlas->data.get();
    this->data_size = this->atlas->size;
    this->texel = vec2(1.0f) / vec2(this->data_size);

    this->offset = offset;
    this->size = size;

    this->unit_size = unit_size;
    this->unit_texel = vec2(this->unit_size) * this->texel;
    this->size_units = this->size / this->unit_size;
}

Pixels TextureAtlasEntry::as_pixels() const {
    return Pixels(this->as_area());
}

TextureArea TextureAtlasEntry::as_area() const {
    return (*this)[AABB2u({ 0, 0 }, this->size_units - 1u)];
}

// re-upload this area to the GPU
void TextureAtlasEntry::upload() const {
    this->atlas->upload(this->as_aabb());
}

TextureAtlas &TextureAtlas::get() {
    static TextureAtlas instance;
    return instance;
}

TextureAtlas::TextureAtlas(const uvec2 &size)
    : size(size) {
    this->data = std::unique_ptr<u8[]>(new u8[size.x * size.y * 4]);
    this->texture = Texture(
        bgfx::createTexture2D(
            this->size.x, this->size.y,
            false, 1,
            PixelFormat::to_texture_format(PIXEL_FORMAT),
            BGFX_SAMPLER_MIN_POINT
                | BGFX_SAMPLER_MAG_POINT
                | BGFX_SAMPLER_U_CLAMP
                | BGFX_SAMPLER_V_CLAMP),
        this->size, this->data.get(), false);
}

TextureAtlas::operator bgfx::TextureHandle() const {
    return this->texture.handle;
}

void TextureAtlas::upload(const AABB2u &box) const {
    // copy texture data into temp buffer
    auto data =
        global.frame_allocator.alloc_span<u8>(
            box.size().x * box.size().y * 4);
    pixels::copy(
        Pixels(&data[0], box.size()),
        Pixels(&this->data[0], box.size(), box.min, this->size));

    bgfx::updateTexture2D(
        this->texture,
        1, 0,
        box.min.x, box.min.y,
        box.size().x, box.size().y,
        bgfx::makeRef(&data[0], data.size_bytes()));
}

void TextureAtlas::tick() {
    for (const auto &anim : this->animations) {
        if ((global.time->ticks % anim.frame_ticks) != 0) {
            continue;
        }

        const auto frame = (global.time->ticks / anim.frame_ticks) % anim.length;
        const auto origin_frame =
            anim.origin + (anim.vertical ?
                uvec2(0, anim.size.y * frame) : uvec2(anim.size.x * frame, 0));

        const auto data =
            global.frame_allocator.alloc_span<u8>(math::prod(anim.size) * 4);

        pixels::copy(
            Pixels(&data[0], anim.size),
            Pixels(&this->data[0], anim.size, origin_frame, this->size));

        bgfx::updateTexture2D(
            this->texture,
            1, 0,
            anim.origin.x, anim.origin.y,
            anim.size.x, anim.size.y,
            bgfx::makeRef(&data[0], data.size_bytes()));
    }
}

TextureAtlasEntry &TextureAtlas::alloc(
    const std::string &label,
    const uvec2 &size,
    const uvec2 &unit_size) {
    if (size.x % unit_size.x != 0
        || size.y % unit_size.y != 0) {
        WARN(
            "Atlas entry {} is not even! size is {} and unit is {}",
            label, size, unit_size);
    }

    // rectangle packing is NP-hard :/
    // find a good place for this entry

    // checks if a box collides with any existing entries
    // returns colliding box if there was one, otherwise nullopt
    auto collides = [&](const AABB2u &box) -> std::optional<AABB2u> {
        for (const auto &[_, entry] : this->entries) {
            if (box.collides(entry->bounds)) {
                return entry->bounds;
            }
        }

        return std::nullopt;
    };

    bool next_x = false, next_y = false;
    auto offset = uvec2(0);

    while (
        offset.x + size.x <= this->size.x &&
        offset.y + size.y <= this->size.y) {
        // check if there is collision at current offset
        const auto box = collides(
            AABB2u(offset, offset + size - uvec2(1)));

        if (!box) {
            // found a spot
            break;
        } else {
            // move to the closest side of the colliding box
            if (next_x || (size.x < size.y && !next_y)) {
                offset.x = box->max.x + 1;
                next_x = false;
            } else {
                offset.y = box->max.y + 1;
                next_y = false;
            }

            const bool
                of_x = offset.x + size.x > this->size.x,
                of_y = offset.y + size.y > this->size.y;

            // double overflow means out of space
            ASSERT(!of_x || !of_y);

            // on overflow, reset up to the next row
            // use next_x/y to ensure that we advance on the opposite axis
            // next time
            if (of_x) {
                offset.x = 0;
                next_y = true;
            }

            if (of_y) {
                offset.y = 0;
                next_x = true;
            }
        }
    }

    LOG("Allocated atlas area {} with sprite size {} (atlas offset {})",
        label, size, offset);

    ASSERT(
        !this->entries.contains(label),
        "duplicate atlas entry {}",
        label);

    return *(this->entries[label] =
        std::make_unique<TextureAtlasEntry>(
            *this, label,
            AABB2u(offset, offset + size - uvec2(1)),
            offset, size, unit_size));
}

TextureAtlasEntry &TextureAtlas::add(
    const std::string &label,
    const u8 *data_i,
    const uvec2 &size_i,
    const uvec2 &unit_size) {
    auto &entry = this->alloc(label, size_i, unit_size);

    // update atlas data
    pixels::copy(
        entry.as_pixels(),
        Pixels(const_cast<u8*>(data_i), size_i));

    entry.upload();

    return entry;
}

void TextureAtlas::animate(
    const TextureAtlasEntry &entry,
    const uvec2 &offset,
    usize length,
    usize frame_ticks,
    bool vertical) {
    Animation anim {
        entry[offset].pixels().min,
        entry.unit_size,
        length,
        frame_ticks,
        vertical
    };

    LOG(
        "Animating {} with length {} @ {} ticks/frame ({})",
        anim.origin,
        length,
        frame_ticks,
        vertical ? "vertical" : "horizontal");

    this->animations.push_back(anim);
}
