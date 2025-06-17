#pragma once

#include "platform/platform.hpp"
#include "util/managed_resource.hpp"
#include "gfx/texture_atlas.hpp"
#include "global.hpp"

struct SpritesheetResource
    : public IManagedResource,
      public BaseManagedResource<SpritesheetResource> {
    using T = const TextureAtlasEntry;
    using CreateFn = std::function<T&(TextureAtlas&)>;
    using InitFn = std::function<void(T&)>;

    CreateFn create_fn;
    std::optional<InitFn> init_fn;
    mutable T *t;
    mutable bool initialized = false;

    // create spritesheet by loading
    SpritesheetResource(
        const std::string &name,
        const std::string &path,
        const uvec2 &sprite_size,
        std::optional<InitFn> init_fn = std::nullopt);

    // create from name lookup
    explicit SpritesheetResource(const std::string &name);

    // create from atlas
    explicit SpritesheetResource(
        CreateFn &&create_fn)
        : create_fn(std::move(create_fn)) {}

    void destroy() override {}
    void attach() override {}

    T &get() const;

    template <typename T>
    inline auto operator[](T &&t) const {
        return this->get()[std::forward<T>(t)];
    }

    template <typename T, std::size_t N>
        requires (std::is_integral<T>::value)
    inline auto operator[](const T(&list)[N]) const {
        static_assert(N == 2, "must use exactly 2 indices");
        return (this->get())[uvec2(list[0], list[1])];
    }
};

struct SpriteResource
    : public IManagedResource,
      public BaseManagedResource<SpriteResource> {
    using T = TextureArea;
    using CreateFn = std::function<T(const TextureAtlas&)>;

    CreateFn create_fn;
    mutable T t;
    mutable bool initialized = false;

    explicit SpriteResource(CreateFn &&create_fn)
        : create_fn(std::move(create_fn)) {}

    void destroy() override {}
    void attach() override {}

    inline T &get() const {
        if (!this->initialized) {
            this->t = create_fn(TextureAtlas::get());
            this->initialized = true;
        }

        return this->t;
    }
};
