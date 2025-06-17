#include "gfx/sprite_resource.hpp"
#include "gfx/texture_atlas.hpp"

// create spritesheet by loading
SpritesheetResource::SpritesheetResource(
    const std::string &name,
    const std::string &path,
    const uvec2 &sprite_size,
    std::optional<InitFn> init_fn)
        : init_fn(init_fn) {
    this->create_fn =
        [=](TextureAtlas &atlas) -> const TextureAtlasEntry& {
            if (atlas.contains(name)) {
                const auto &entry = atlas[name];
                if (entry.path
                        && *entry.path == path
                        && entry.unit_size == sprite_size) {
                    LOG("Getting existing spritesheet {}", name);
                    return entry;
                }

                ASSERT(
                    false,
                    "Attempt to load two spritesheets with same label {}",
                    name);
            }

            const auto [size, data] =
                gfx::load_texture_data(
                    global.platform->resources_path + "/" + path)
                    .unwrap();
            auto &result = atlas.add(name, data, size, sprite_size);
            result.path = path;
            std::free(data);
            return result;
        };
}

// create from name lookup
SpritesheetResource::SpritesheetResource(const std::string &name)
    : create_fn(
        [=](const auto &atlas) -> const auto& { return atlas[name]; }) {}


SpritesheetResource::T &SpritesheetResource::get() const {
    if (!this->initialized) {
        this->t = &create_fn(TextureAtlas::get());
        if (this->init_fn) {
            (*this->init_fn)(*this->t);
        }
        this->initialized = true;
    }

    return *this->t;
}
