#pragma once

#include "util/types.hpp"
#include "util/util.hpp"

#include "gfx/bgfx.hpp"
#include "gfx/texture.hpp"
#include "gfx/util.hpp"
#include "gfx/texture.hpp"
#include "gfx/uniform.hpp"

// forward declaration
struct Renderer;

struct Program {
    bgfx::ProgramHandle handle = { bgfx::kInvalidHandle };

    // TODO: this should not be shared_ptr
    std::unordered_map<std::string, std::shared_ptr<Uniform>> uniforms;

    std::string name;

    Program() = default;

    Program(
        const std::string &name,
        Renderer &renderer,
        const std::string &vs_path,
        const std::string &fs_path);

    Program(const Program &other) = delete;

    Program(Program &&other) : handle(other.handle) {
        *this = std::move(other);
    }

    Program &operator=(const Program &other) = delete;

    Program &operator=(Program &&other) {
        this->handle = other.handle;
        other.handle = { bgfx::kInvalidHandle };
        this->uniforms = other.uniforms;
        this->name = other.name;
        return *this;
    }

    ~Program();

    inline void reset() const {
        for (const auto &[_, u] : this->uniforms) {
            u->set = false;
        }
    }

    inline operator bgfx::ProgramHandle() const {
        return this->handle;
    }

    inline bool has(const std::string &name) const {
        return this->uniforms.contains(name);
    }

    inline bool is_set(const std::string &name) const {
        return this->uniforms.at(name)->set;
    }

    inline void set(
        const std::string &name, const UniformValue &v, u16 num = 1) const {
        auto &uniform = *this->uniforms.at(name);
        uniform.set = true;
        bgfx::setUniform(uniform.handle, &v.value, num);
    }

    inline bool try_set(
        const std::string &name, const UniformValue &v, u16 num = 1) const {
        if (!this->uniforms.contains(name)) {
            return false;
        }

        auto &uniform = *this->uniforms.at(name);
        uniform.set = true;
        bgfx::setUniform(uniform.handle, &v.value, num);
        return true;
    }

    inline void set(
        const std::string &name, void *value, u16 num = 1) const {
        auto &uniform = *this->uniforms.at(name);
        uniform.set = true;
        bgfx::setUniform(uniform.handle, value, num);
    }

    inline bool try_set(
        const std::string &name, void *value, u16 num = 1) const {
        if (!this->uniforms.contains(name)) {
            return false;
        }

        auto &uniform = *this->uniforms.at(name);
        uniform.set = true;
        bgfx::setUniform(uniform.handle, value, num);
        return true;
    }

    template<typename T>
        requires (!std::is_same_v<T, void *>
                  && !std::is_convertible_v<T, const Texture&>)
    inline void set(
        const std::string &name, const T &value, u16 num = 1) const {
        this->set_and_pad(name, value, num);
    }

    template<typename T>
        requires (!std::is_same_v<T, void *>
                  && !std::is_convertible_v<T, const Texture&>)
    inline bool try_set(
        const std::string &name, const T &value, u16 num = 1) const {
        if (!this->uniforms.contains(name)) {
            return false;
        }
        this->set_and_pad(name, value, num);
        return true;
    }

    // set override for textures
    inline void set(
        const std::string &name, u8 stage, const Texture &texture) const {
        auto &uniform = *this->uniforms.at(name);
        uniform.set = true;
        bgfx::setTexture(stage, uniform.handle, texture);
    }

    // try_set override for textures
    inline bool try_set(
        const std::string &name, u8 stage, const Texture &texture) const {
        auto it = this->uniforms.find(name);
        if (it == this->uniforms.end()) {
            return false;
        }

        this->set(name, stage, texture);
        return true;
    }

private:
    template <typename T>
    inline void set_and_pad(
        const std::string &name,
        const T &value,
        u8 num) const {
        auto &uniform = *this->uniforms.at(name);
        uniform.set = true;

        if constexpr (std::is_same_v<T, vec3>) {
            const auto v = vec4(value, 0.0f);
            bgfx::setUniform(uniform.handle, &v, num);
        } else if constexpr (std::is_same_v<T, vec2>) {
            const auto v = vec4(value, 0.0f, 0.0f);
            bgfx::setUniform(uniform.handle, &v, num);
        } else {
            bgfx::setUniform(uniform.handle, &value, num);
        }
    }
};
