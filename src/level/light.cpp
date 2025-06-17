#include "level/light.hpp"
#include "level/level.hpp"
#include "gfx/program.hpp"
#include "gfx/renderer_resource.hpp"
#include "global.hpp"

// light shader
#include "light/light.sc"

struct Node {
    ivec3 pos;
    u8 value;

    Node() = default;
    Node(ivec3 pos, u8 value) : pos(pos), value(value) {}

    template<std::size_t I>
    std::tuple_element_t<I, Node> get() const {
        if constexpr (I == 0) return this->pos;
        if constexpr (I == 1) return this->value;
    }
};

namespace std {
template <>
struct tuple_size<Node>
    : integral_constant<size_t, 2> {};

template<>
struct tuple_element<0, Node> {
    using type = decltype(Node::pos);
};

template<>
struct tuple_element<1, Node> {
    using type = decltype(Node::value);
};
}

// circular buffer, acting as queue for light nodes
// rationale for a queue over a stack (BFS over DFS) is that a queue will allow
// for higher lighting values to always be set first, therefore preventing the
// re-setting of lights over and over again under propagation
struct NodeQueue {
    static constexpr usize MAX = 63356;

    std::array<Node, MAX> queue;
    usize i = 0, j = 0;

    NodeQueue() = default;

    inline usize size() {
        return this->j - this->i;
    }

    inline void push(const Node &n) {
        this->queue[this->j] = n;
        this->j = (this->j + 1) % MAX;
    }

    inline Node pop() {
        Node res = this->queue[this->i];
        this->i = (this->i + 1) % MAX;
        return res;
    }
};

static void add_propagate(NodeQueue &queue, Level &level) {
    while (queue.size() != 0) {
        const auto [pos, light] = queue.pop();

        if (light <= 1) {
            continue;
        }

        UNROLL(6)
        for (const auto d : Direction::ALL) {
            const auto pos_n = pos + Direction::to_ivec3(d);
            auto proxy_n = level.raw[pos_n];

            if (!proxy_n.present()) {
                continue;
            }

            const auto data_n = proxy_n.get();
            const auto light_n = Chunk::LightData::from(data_n);
            const u8 new_light_n = light - 1;

            if (light_n >= new_light_n) {
                continue;
            }

            // try to avoid lookup into tile array if tile is air
            const auto tile_n = Chunk::TileData::from(data_n);
            if (tile_n != 0
                    && (Tiles::get()[tile_n].transparency_type() !=
                            Tile::Transparency::ON)) {
                continue;
            }

            Chunk::LightData::set_no_mesh(proxy_n, new_light_n);

            if (new_light_n > 1) {
                queue.push({ pos_n, new_light_n });
            }
        }
    }
}

static void remove_propagate(
    NodeQueue &queue, NodeQueue &prop, Level &level) {
    while (queue.size() != 0) {
        const auto [pos, light] = queue.pop();

        if (light == 0) {
            continue;
        }

        UNROLL(6)
        for (const auto d : Direction::ALL) {
            const auto pos_n = pos + Direction::to_ivec3(d);
            const auto proxy_n = level.raw[pos_n];

            if (!proxy_n.present()) {
                continue;
            }

            const auto light_n = Chunk::LightData::from(proxy_n);

            if (light_n == 0) {
                continue;
            } else if (light_n < light) {
                Chunk::LightData::set_no_mesh(proxy_n, 0);
                queue.push({ pos_n, light_n });
            } else if (light_n >= light) {
                prop.push({ pos_n, light_n });
            }
        }
    }
}

void Light::add(
    Level &level, const ivec3 &pos, u8 light) {
    if (!level.contains(pos)) {
        return;
    }

    NodeQueue queue;
    level.light[pos] = light;
    queue.push(Node(pos, light));
    add_propagate(queue, level);
}

// TODO: what happens if a weak light is overwhelmed by this light?
// need a mechanisim for re-adding light from weak sources (call into level)
void Light::remove(
    Level &level, const ivec3 &pos, u8 light) {
    if (!level.contains(pos)) {
        return;
    }

    NodeQueue queue, prop;
    level.light[pos] = 0;
    queue.push(Node(pos, light));
    remove_propagate(queue, prop, level);

    // light values may have changed after addition into the queue
    for (usize i = 0; i < prop.size(); i++) {
        auto &n = prop.queue[i];
        n.value = level.light[n.pos];
    }

    add_propagate(prop, level);
}

void Light::update(
    Level &level, const ivec3 &pos) {
    NodeQueue queue;

    for (const auto &d : Direction::ALL) {
        const auto p = pos + Direction::to_ivec3(d);
        queue.push(Node(p, level.light[p]));
    }

    add_propagate(queue, level);
}

// light shader uniform information
// NOTE: must match exactly light/light.sc layout
struct UniformInfo {
    union { struct { vec3 position; f32 present; }; vec4 _u0; };
    union {
        struct {
            f32 constant, linear, quadratic, shadow;
        };
        vec4 _u1;
    };
    union { vec3 ambient; vec4 _u2; };
    union { vec3 diffuse; vec4 _u3; };
    union { vec3 specular; vec4 _u4; };

    UniformInfo() {
        std::memset(this, 0, sizeof(*this));
    }

    explicit UniformInfo(const Light &light)
        : position(light.pos),
          present(1.0f),
          constant(light.att_constant),
          linear(light.att_linear),
          quadratic(light.att_quadratic),
          shadow(light.shadow),
          ambient(0.0),
          diffuse(light.color),
          specular(light.color) {}
};

using UniformBufferType = RDResource<bgfx::DynamicIndexBufferHandle>;
static auto uniform_buffer =
    RendererResource<UniformBufferType>(
        []() {
            constexpr auto size = MAX_LIGHTS * sizeof(UniformInfo);
            return gfx::as_bgfx_resource(
                bgfx::createDynamicIndexBuffer(
                    size / sizeof(u16), BGFX_BUFFER_COMPUTE_READ));
        });

void Light::set_uniforms(
    const Program &program,
    std::span<Light> lights) {
    ASSERT(
        lights.size() <= MAX_LIGHTS, "Too many lights", false);

    static_assert(LIGHT_SIZE_VEC4 * sizeof(vec4) == sizeof(UniformInfo));

    const auto &buffer = uniform_buffer.get();
    const auto data =
        global.frame_allocator.alloc_span<UniformInfo>(MAX_LIGHTS);
    for (usize i = 0; i < lights.size(); i++) {
        data[i] = UniformInfo(lights[i]);
    }
    bgfx::update(
        buffer, 0, bgfx::makeRef(&data[0], data.size_bytes()));
    bgfx::setBuffer(BUFFER_INDEX_LIGHT, buffer, bgfx::Access::Read);
}
