#pragma once

#include "util/util.hpp"
#include "util/types.hpp"
#include "util/math.hpp"
#include "util/list.hpp"
#include"gfx/bgfx.hpp"

struct Program;

struct Uniform {
    std::string name;
    bgfx::UniformType::Enum type;
    u16 num;
    bgfx::UniformHandle handle = { bgfx::kInvalidHandle };
    bool set = false;
    bool destroy = false;

    Uniform() = default;
    Uniform(const Uniform &other) = delete;
    Uniform &operator=(const Uniform &other) = delete;

    Uniform(
        const std::string &name,
        bgfx::UniformType::Enum type,
        u16 num,
        bgfx::UniformHandle handle,
        bool destroy = true)
        : name(name),
          type(type),
          num(num),
          handle(handle),
          destroy(destroy) {};

    Uniform(Uniform &&other) {
        *this = std::move(other);
    }

    Uniform &operator=(Uniform &&other) {
        this->name = other.name;
        this->type = other.type;
        this->num = other.num;
        this->handle = other.handle;
        this->destroy = other.destroy;
        other.handle = { bgfx::kInvalidHandle };
        other.destroy = false;
        return *this;
    }

    ~Uniform() {
        if (this->destroy && bgfx::isValid(this->handle)) {
            bgfx::destroy(this->handle);
        }
    }

    inline operator bgfx::UniformHandle() {
        return this->handle;
    }

    inline auto operator<=>(const Uniform &rhs) {
        return this->handle.idx <=> rhs.handle.idx;
    }
};

enum UniformKind {
    UK_UNDEFINED,
    UK_VEC4,
    UK_MAT4,
    UK_MAT3,
    UK_MAT2,
};

// TODO: samplers?
struct UniformValue {
    UniformKind kind = UK_UNDEFINED;

    union {
        mat4 m4;
        mat3 m3;
        mat2 m2;
        vec4 v4;

        // sizeof largest element
        f32 value[sizeof(mat4) / sizeof(f32)];
    };

    UniformValue() = default;

    UniformValue(const mat4 &m4)
        : kind(UK_MAT4),
          m4(m4) {}

    UniformValue(const mat3 &m3)
        : kind(UK_MAT3),
          m3(m3) {}

    UniformValue(const mat2 &m2)
        : kind(UK_MAT2),
          m2(m2) {}

    UniformValue(const vec4 &v4)
        : kind(UK_VEC4),
          v4(v4) {}

    UniformValue(const vec3 &v3)
        : kind(UK_VEC4),
          v4(v3, 0.0f) {}
};

struct UniformValueList {
    SmallList<std::tuple<std::string, UniformValue>, 8> vs;

    UniformValueList() = default;
    explicit UniformValueList(Allocator *allocator)
        : vs(allocator) {}

    inline void set(std::string_view name, const UniformValue &v) {
        for (auto &[n, _] : this->vs) {
            if (name == n) {
                ASSERT(false, "already have uniform with name {}", name);
            }
        }

        this->vs.emplace(std::string(name), v);
    }

    inline void append(const UniformValueList &v) {
        this->vs.push(v.vs);
    }

    void try_apply(const Program &program) const;

    void apply(const Program &program) const;
};
