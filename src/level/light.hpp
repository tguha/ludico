#pragma once

#include "util/assert.hpp"
#include "util/types.hpp"
#include "util/util.hpp"
#include "util/math.hpp"
#include "util/aabb.hpp"

// forward declaration
struct Level;
struct Program;

struct Light {
    // world position
    vec3 pos;

    // color, both diffuse and specular
    vec3 color;

    // 0-15
    u8 value;

    f32 att_constant = 1.0f,
        att_quadratic = 0.20f,
        att_linear = 0.22f;

    bool shadow;

    Light() = default;
    Light(const vec3 &color, u8 value, bool shadow = true)
        : Light(vec3(0), color, value, shadow) {}
    Light(const vec3 &pos, const vec3 &color, u8 value, bool shadow = true)
        : pos(pos), color(color), value(value), shadow(shadow) {}

    std::string to_string() const {
        return fmt::format("Light(pos={},color={})", this->pos, this->color);
    }

    static void add(Level &level, const ivec3 &pos, u8 light);
    static void remove(Level &level, const ivec3 &pos, u8 light);
    static void update(Level &level, const ivec3 &pos);

    // uploads light uniforms for the program
    static void set_uniforms(
        const Program &program,
        std::span<Light> lights);
};

template <usize N>
struct NLightArray {
    static constexpr auto _N = N;

    usize count = 0;
    std::array<Light, N> arr;

    NLightArray() = default;

    NLightArray(const std::optional<Light> &l)
        : count(l ? 1 : 0),
          arr(count ? decltype(arr)({ *l }) : decltype(arr)()) {}

    NLightArray(const Light &l)
        : count(1),
          arr({ l }) {}

    NLightArray(std::initializer_list<Light> lights)
        : count(lights.size()) {
        ASSERT(lights.size() <= N);
        std::copy(lights.begin(), lights.end(), this->arr.begin());
    }

    inline auto size() const { return count; }

    inline auto begin() const { return arr.begin(); }

    inline auto end() const { return arr.begin() + this->count; }

    inline auto begin() { return arr.begin(); }

    inline auto end() { return arr.begin() + this->count; }

    inline auto &operator[](auto i) const { return arr[i]; }

    inline auto &operator[](auto i) { return arr[i]; }

    inline bool try_add(const Light &l) {
        if (this->count >= _N) {
            return false;
        }

        (*this)[this->count++] = l;
        return true;
    }
};

// use reasonable light array size as default
using LightArray = NLightArray<4>;
