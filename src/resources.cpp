#include "resources.hpp"

#include <toml.hpp>

#include "gfx/renderer.hpp"
#include "gfx/texture_atlas.hpp"
#include "util/util.hpp"

static void load_shaders(
    const std::string &base_path,
    Renderer &renderer) {
    // load all programs in shaders directory
    const auto full_path = fmt::format("{}/shaders", base_path);
    for (const auto &p : file::list_files(full_path).unwrap()) {
        if (!file::is_directory(p).unwrap()) {
            continue;
        }


        const auto files = file::list_files(p).unwrap();

        // search for all shaders which should exist
        std::unordered_set<std::string> programs;
        for (const auto &f : files) {
            const auto [base, filename, ext] = file::split_path(f);

            if (filename.find('.') == std::string::npos
                && (filename.starts_with("vs_") ||
                    filename.starts_with("fs_"))) {
                programs.insert(filename.substr(3));
            }
        }

        // load a program for each vs_, fs_ combination which should be found
        for (const auto &name : programs) {
            const auto
                vs = fmt::format("{}/vs_{}.sc", p, name),
                fs = fmt::format("{}/fs_{}.sc", p, name);

            if (!file::exists(vs)) {
                WARN("Missing vertex shader {}", vs);
                continue;
            }

            if (!file::exists(fs)) {
                WARN("Missing fragment shader {}", fs);
                continue;
            }

            renderer.programs[name] = Program(name, renderer, vs, fs);
        }
    }
}

void resources::load(
    const std::string &base_path,
    Renderer &renderer) {
    load_shaders(base_path, renderer);
}
