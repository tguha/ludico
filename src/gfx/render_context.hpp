#pragma once

#include "gfx/uniform.hpp"
#include "gfx/multi_mesh.hpp"
#include "util/util.hpp"
#include "util/types.hpp"
#include "util/math.hpp"
#include "util/list.hpp"
#include "ext/inplace_function.hpp"
#include "constants.hpp"
#include "entity/util.hpp"

struct Program;
struct RenderState;
struct GenericMultiMeshEntry;

// TODO: allow precalculated normal matrix?
// single part of model to be rendered either via instancing or individually
struct ModelPart {
    const MultiMeshEntry *entry;

    // model matrix
    mat4 model;

    // graphics flags GFX_FLAG_*
    u8 gfx_flags = GFX_FLAG_NONE;

    // passes which should include this model part
    u8 passes = RENDER_FLAG_PASS_ALL_3D;

    // is which is associated with this model part
    EntityId entity_id = 0;
};

struct RenderGroup;
struct RenderContext;

struct RenderCtxFn {
    using Fn = InplaceFunction<void(const RenderGroup&, RenderState), 32>;
    Fn fn;
    u8 passes = RENDER_FLAG_PASS_ALL_3D;

    RenderCtxFn() = default;
    RenderCtxFn(Fn &&fn)
        : fn(std::move(fn)) {}
    RenderCtxFn(Fn &&fn, u8 passes)
        : fn(std::move(fn)),
          passes(passes) {}
};

struct RenderGroup {
    struct InstanceData;

    enum Flags {
        // if true, model parts are instanced
        INSTANCED = (1 << 0)
    };

    // context this group belongs to
    RenderContext *ctx;

    // unique id of this group
    u16 id = std::numeric_limits<u16>::max();

    // gfx flags which apply to all group members
    u8 group_gfx_flags = 0;

    // passes which apply to all group members
    u8 group_passes = 0;

    // flags which are applied to RenderState::flags for group
    u64 group_render_flags = 0;

    // flags which are applied to RenderState::bgfx
    u64 group_bgfx_flags = 0;

    // flags for render group
    u64 group_flags = 0;

    // entity id for this group
    EntityId group_entity_id = NO_ENTITY;

    // uniforms to try_set for every program used by this group
    UniformValueList uniforms;

    // program with which group should be rendered
    // can be null if this group has no model parts
    const Program *program = nullptr;

    // model parts in this group
    BlockList<ModelPart, 16> model_parts;

    // render functions in this group
    BlockList<RenderCtxFn, 16> render_fns;

    // TODO: doc
    InstanceData *instance_data = nullptr;

    RenderGroup() = default;
    explicit RenderGroup(RenderContext &ctx);

    // inherit properties from another render group
    void inherit(const RenderGroup &other);
};

// single-frame rendering context
struct RenderContext {
    // ID of group whose model parts are auto-instanced
    static constexpr auto GROUP_ID_DEFAULT_INSTANCE = 0;

    // allocator with (at least) frame lifetime
    Allocator *allocator;

    // all groups, ordered by id
    BlockList<RenderGroup, 16> groups;

    // group stack controlled by push/pop_group
    List<RenderGroup*> group_stack;

    RenderContext() = default;
    explicit RenderContext(Allocator *allocator);

    // push a model part for rendering to current group
    void push(const ModelPart &model_part);

    // push multiple model parts to current group
    void push(std::span<const ModelPart> model_parts) {
        for (const auto &p : model_parts) {
            this->push(p);
        }
    }

    // push a render function to be called with each render() call to current
    // group
    void push(RenderCtxFn fn);

    // prepare render context for rendering after group traversal/gathering is
    // done
    void prepare();

    // perform a render pass with this context
    void pass(RenderState render_state);

    // create a new group, returns reference
    RenderGroup &push_group();

    // push an existing group
    RenderGroup &push_group(RenderGroup &group);

    // push an existing group by id
    RenderGroup &push_group(u16 id);

    // pop last group
    void pop_group();

    // get group by id
    inline RenderGroup &get_group(u16 id) {
        ASSERT(
            id < this->groups.size(),
            "group id {} out of range {}",
            id, this->groups.size());
        return this->groups[id];
    }

    // get current top group
    inline RenderGroup &top_group() {
        return *this->group_stack[this->group_stack.size() - 1];
    }
};
