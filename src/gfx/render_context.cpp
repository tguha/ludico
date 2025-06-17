#include "gfx/render_context.hpp"
#include "gfx/util.hpp"
#include "gfx/renderer.hpp"
#include "gfx/multi_mesh.hpp"
#include "gfx/multi_mesh_instancer.hpp"
#include "global.hpp"

// TODO: cleanup
// TODO: doc
struct RenderGroup::InstanceData {
    // TODO: doc
    std::unordered_map<const MultiMesh*, MultiMeshInstanceBuffer>
        buffers;

    std::vector<MultiMeshInstancer::RenderData> data;
};

RenderGroup::RenderGroup(RenderContext &ctx)
    : ctx(&ctx),
      uniforms(ctx.allocator),
      model_parts(ctx.allocator),
      render_fns(ctx.allocator) {}

void RenderGroup::inherit(const RenderGroup &other) {
    this->group_gfx_flags = other.group_gfx_flags;
    this->group_passes = other.group_passes;
    this->group_render_flags = other.group_render_flags;
    this->group_bgfx_flags = other.group_bgfx_flags;
    this->group_flags = other.group_flags;
    this->group_entity_id = other.group_entity_id;
    this->uniforms = other.uniforms;
    this->program = other.program;
}

RenderContext::RenderContext(Allocator *allocator)
	: allocator(allocator),
	  groups(allocator),
	  group_stack(allocator) {
	// push base instancing group
	auto &instance_group = this->push_group();
	ASSERT(instance_group.id == GROUP_ID_DEFAULT_INSTANCE);
    instance_group.group_bgfx_flags = BGFX_FLAGS_DEFAULT;
	instance_group.group_passes =
        RENDER_FLAG_PASS_BASE
        | RENDER_FLAG_PASS_SHADOW
        | RENDER_FLAG_PASS_TRANSPARENT;
	instance_group.program = &Renderer::get().programs["model_instanced"];
    instance_group.group_flags |= RenderGroup::INSTANCED;
}

void RenderContext::push(const ModelPart &model_part) {
    ASSERT(model_part.entry);
	this->top_group().model_parts.push(model_part);
}

void RenderContext::push(RenderCtxFn fn) {
	this->top_group().render_fns.push(fn);
}

void RenderContext::prepare() {
	// if group_stack size != 1, something went wrong
	ASSERT(
		this->group_stack.size() == 1,
		"uneven group stack");

    // allocate for all instancing groups
    for (auto &g : this->groups) {
        if (!(g.group_flags & RenderGroup::INSTANCED)) {
            continue;
        }

        // allocate instance data
        auto &instance_data =
            *this->allocator->alloc<RenderGroup::InstanceData>();
        g.instance_data = &instance_data;

        // populate with model parts
        for (const auto &p : g.model_parts) {
            const auto &mesh = p.entry->generic_parent();

            MultiMeshInstanceBuffer *buffer;

            // get or create buffer
            auto it = instance_data.buffers.find(&mesh);
            if (it == instance_data.buffers.end()) {
                buffer =
                    &instance_data.buffers.try_emplace(
                        &mesh,
                        this->allocator,
                        mesh).first->second;
            } else {
                buffer = &it->second;
            }

            buffer->push(
                ModelInstanceData {
                    .model = p.model,
                    .normal = math::normal_matrix(p.model),
                    .flags = f32(p.gfx_flags), // TODO
                    .id = f32(p.entity_id) // TODO
                },
                p.entry);
        }

        // upload all buffers into instancer
        for (const auto &p : instance_data.buffers) {
            const auto &[_, b] = p;
            instance_data.data.push_back(
                Renderer::get().mm_instancer->push(b));
        }
    }
}

void RenderContext::pass(RenderState render_state) {
	// mask to current pass
	const auto pass = render_state.flags & RENDER_FLAG_PASS_ALL_3D;

	for (const auto &g : this->groups) {
		// check if group is in this render pass
		if (!(g.group_passes & pass)) {
			continue;
		} else if (
            g.model_parts.empty()
            && g.render_fns.empty()) {
            // nothing to render
            continue;
        }

		RenderState rs = render_state;
		rs.flags |= g.group_render_flags;
		rs.bgfx |= g.group_bgfx_flags;

        if (g.program) {
		    g.uniforms.try_apply(*g.program);
        } else {
            ASSERT(
                g.model_parts.empty(),
                "render groups with model parts must specify a program ({})",
                g.id);
        }

		// run basic render functions
		for (const auto &f : g.render_fns) {
            if (!(f.passes & pass)) {
                continue;
            }

			f.fn(g, rs);
		}

        if (g.group_flags & RenderGroup::INSTANCED) {
            ASSERT(g.instance_data);
            for (const auto &d : g.instance_data->data) {
                Renderer::get().mm_instancer->render(
                    d,
                    rs,
                    *g.program);
            }
        } else {
            // render model parts directly
            for (const auto &p : g.model_parts) {
                // check if part is in this render pass
                if (!(g.group_passes & pass)) {
                    continue;
                }

                // set "u_flags_id" if not already set
                g.program->try_set(
                    "u_flags_id",
                    vec4(
                        g.group_gfx_flags | p.gfx_flags,
                        p.entity_id != NO_ENTITY ?
                            p.entity_id : g.group_entity_id,
                        0.0f,
                        1.0f));

                p.entry->render(
                    *g.program,
                    p.model,
                    rs);
            }
        }
	}
}

RenderGroup &RenderContext::push_group() {
	auto &group = this->groups.emplace(*this);
	group.id = this->groups.size() - 1;
	this->group_stack.push(&group);
	return group;
}


RenderGroup &RenderContext::push_group(RenderGroup &group) {
    this->group_stack.emplace(&group);
	return group;
}

RenderGroup &RenderContext::push_group(u16 id) {
    ASSERT(
        id < this->groups.size(),
        "group id {} out of range {}",
        id, this->groups.size());
    return *this->group_stack.push(&this->groups[id]);
}

void RenderContext::pop_group() {
	this->group_stack.pop();
}
