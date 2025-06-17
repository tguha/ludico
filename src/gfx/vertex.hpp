#pragma once

#include "util/types.hpp"
#include "util/util.hpp"
#include "util/math.hpp"
#include "util/type_registry.hpp"
#include "gfx/bgfx.hpp"

template <typename V>
concept VertexHasColor = requires (V &v) {
    { v.color } -> std::same_as<vec4&>;
};

template <typename V>
concept VertexHasNormal = requires (V &v) {
    { v.normal } -> std::same_as<vec3&>;
};

template <typename V>
concept VertexHasST = requires (V &v) {
    { v.st } -> std::same_as<vec2&>;
};

template <typename V>
concept VertexHasSTSpecular = requires (V &v) {
    { v.st_specular } -> std::same_as<vec2&>;
};

template <typename V>
struct VertexType {
    // implemented in macros, see below
    static void init_layout();
};

struct VertexTypeManager {
    static VertexTypeManager &instance();

    void init();

    template <typename T>
    void register_type() {
        VertexType<T>::init_layout();
    }
};

using VertexTypeRegistry = TypeRegistry<VertexTypeManager, types::Empty>;
template <> VertexTypeRegistry &VertexTypeRegistry::instance();

template <typename V>
using VertexTypeRegistrar = TypeRegistrar<VertexTypeRegistry, V>;

#define DECL_VERTEX_TYPE_HEADER(_v)                                            \
    template <> void VertexType<_v>::init_layout();

#define DECL_VERTEX_TYPE(_v)                                                   \
    template struct VertexType<_v>;                                            \
    bgfx::VertexLayout _v::layout = bgfx::VertexLayout();                      \
    static auto CONCAT(_v, __COUNTER__) = VertexTypeRegistrar<_v>();           \
    template <> void VertexType<_v>::init_layout()

struct VertexDummy16 : public VertexType<VertexDummy16> {
    static bgfx::VertexLayout layout;
    vec4 _;
} PACKED;
DECL_VERTEX_TYPE_HEADER(VertexDummy16)

struct VertexPosition : public VertexType<VertexPosition> {
    static bgfx::VertexLayout layout;
    vec3 position;

    VertexPosition() = default;

    explicit VertexPosition(const vec3 &position)
        : position(position) {}
} PACKED;
DECL_VERTEX_TYPE_HEADER(VertexPosition)

struct VertexTexture {
    static bgfx::VertexLayout layout;
    vec3 position;
    vec2 st;

    VertexTexture() = default;

    VertexTexture(const vec3 &position, const vec2 &st)
        : position(position), st(st) {}
} PACKED;
DECL_VERTEX_TYPE_HEADER(VertexTexture)

struct VertexColor {
    static bgfx::VertexLayout layout;
    vec3 position;
    vec4 color;

    VertexColor() = default;

    VertexColor(const vec3 &position, const vec4 &color)
        : position(position), color(color) {}
} PACKED;
DECL_VERTEX_TYPE_HEADER(VertexColor)

struct VertexColorNormal {
    static bgfx::VertexLayout layout;
    vec3 position;
    vec4 color;
    vec3 normal;

    VertexColorNormal() = default;

    VertexColorNormal(
        const vec3 &position,
        const vec4 &color,
        const vec3 &normal)
        : position(position), color(color), normal(normal) {}
} PACKED;
DECL_VERTEX_TYPE_HEADER(VertexColorNormal)

struct VertexTextureNormal {
    static bgfx::VertexLayout layout;
    vec3 position;
    vec2 st;
    vec3 normal;

    VertexTextureNormal() = default;

    VertexTextureNormal(
        const vec3 &position,
        const vec2 &st,
        const vec3 &normal)
        : position(position), st(st), normal(normal) {}
} PACKED;
DECL_VERTEX_TYPE_HEADER(VertexTextureNormal)

struct VertexTextureSpecularNormal {
    static bgfx::VertexLayout layout;
    vec3 position;
    vec2 st;
    vec2 st_specular;
    vec3 normal;

    VertexTextureSpecularNormal() = default;

    VertexTextureSpecularNormal(
        const vec3 &position,
        const vec2 &st,
        const vec2 &st_specular,
        const vec3 &normal)
        : position(position),
          st(st),
          st_specular(st_specular),
          normal(normal) {}
} PACKED;
DECL_VERTEX_TYPE_HEADER(VertexTextureNormal)

struct VertexTextureColor {
    static bgfx::VertexLayout layout;
    vec3 position;
    vec2 st;
    vec4 color;

    VertexTextureColor() = default;

    VertexTextureColor(
        const vec3 &position,
        const vec2 &st,
        const vec4 &color)
        : position(position), st(st), color(color) {}
} PACKED;
DECL_VERTEX_TYPE_HEADER(VertexTextureColor)
