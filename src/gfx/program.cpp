#include "gfx/program.hpp"
#include "gfx/renderer.hpp"

Program::Program(
    const std::string &name,
    Renderer &renderer,
    const std::string &vs_path,
    const std::string &fs_path) {
    this->name = name;

    LOG("Loading program ({}) {} + {}", this->name, vs_path, fs_path);

    auto
        vs = gfx::load_shader(gfx::get_platform_shader(vs_path)).unwrap(),
        fs = gfx::load_shader(gfx::get_platform_shader(fs_path)).unwrap();

    this->handle = bgfx::createProgram(vs, fs, true);
    ASSERT(bgfx::isValid(this->handle), "Error loading shader");

    LOG("  handle: {}", this->handle.idx);

    const auto
        n_vs = bgfx::getShaderUniforms(vs),
        n_fs = bgfx::getShaderUniforms(fs);

    std::vector<bgfx::UniformHandle> handles(n_vs + n_fs);
    if (n_vs > 0) {
        bgfx::getShaderUniforms(vs, &handles[0], n_vs);
    }

    if (n_fs > 0) {
        bgfx::getShaderUniforms(fs, &handles[n_vs], n_fs);
    }

    for (const auto handle : handles) {
        bgfx::UniformInfo info;
        bgfx::getUniformInfo(handle, info);

        // check if uniform already exists in renderer
        if (renderer.uniforms.contains(handle.idx)) {
            this->uniforms[info.name] = renderer.uniforms[handle.idx];
            LOG("  (already exists)");
        } else {
            auto uniform =
                std::make_shared<Uniform>(
                    info.name, info.type, info.num, handle, false);
            renderer.uniforms[handle.idx] = uniform;
            this->uniforms[info.name] = uniform;
        }

        LOG("  uniform: {}", info.name);
    }
}

Program::~Program() {
    if (!bgfx::isValid(this->handle)) {
        return;
    }

    LOG("Destroying program {}", this->handle.idx);
    bgfx::destroy(this->handle);
}
