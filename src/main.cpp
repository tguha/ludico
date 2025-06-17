#include <backward.hpp>

#include "gfx/renderer.hpp"
#include "input/controls.hpp"
#include "input/input.hpp"
#include "platform/platform.hpp"
#include "sound/sound.hpp"
#include "ui/ui_container.hpp"
#include "ui/ui_root.hpp"
#include "util/rand.hpp"
#include "util/time.hpp"
#include "util/toml.hpp"
#include "tile/tile.hpp"
#include "item/item.hpp"
#include "state/state.hpp"
#include "state/state_game.hpp"
#include "init_registry.hpp"
#include "entry.hpp"
#include "locale.hpp"
#include "resources.hpp"
#include "constants.hpp"
#include "global.hpp"

// TODO: remove
#include "serialize/serialize.hpp"

// TODO: other platforms
// include platform, based on game target
#ifdef GAME_TARGET_osx_arm64
#include "platform/glfw/platform_glfw.hpp"
#endif

// global state, see global.hpp
Global global;

int main(int argc, char *argv[]) {
    // set up stack trace printing on crash
    backward::SignalHandling sh;
    if (!sh.loaded()) {
        WARN("Could not load backward");
    }

    // declare first, must be last to be destructed!
    Platform platform;
    global.platform = &platform;

    global.allocator =
        BasicAllocator();
    global.frame_allocator =
        BumpAllocator(&global.allocator, 4 * 1024 * 1024);
    global.tick_allocator =
        BumpAllocator(&global.allocator, 4 * 1024 * 1024);
    global.long_allocator =
        LongAllocator<2>(&global.allocator, 4 * 1024 * 1024);

    Time time([]() {
        return
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch())
            .count();
    });
    global.time = &time;

    platform.resources_path = "res";
    platform.log_out = &std::cout;
    platform.log_err = &std::cerr;

    // load reflection engine
    archimedes::set_collision_callback(
        [](const auto &a, const auto &b) {
            ASSERT(
                a.name() == b.name(),
                "bad collision for {} and {}",
                a.name(), b.name());
            return a;
        });
    archimedes::load();

    // load settings/resources/strings manifests
    const auto settings =
        toml::parse(
            file::read_string(
                platform.resources_path + "/defaults.toml").unwrap());
    global.settings = settings.as_table();

    Locale::instance() =
        Locale::from_toml(
            fmt::format(
                "{}/strings/{}.toml",
                platform.resources_path,
                *(*global.settings)["locale"].value<std::string>()));

    platform.window = std::make_unique<GLFWWindow>(
        uvec2(
            (*global.settings)["gfx"]["width"].value_or(1280),
            (*global.settings)["gfx"]["height"].value_or(720)),
        "GAME");
    platform.inputs["mouse"] = std::make_unique<GLFWMouse>(
        *dynamic_cast<GLFWWindow*>(global.platform->window.get()));
    platform.inputs["keyboard"] = std::make_unique<GLFWKeyboard>(
        *dynamic_cast<GLFWWindow*>(global.platform->window.get()));

    // hide cursor, it is drawn manually
    if (auto cursor = platform.window->cursor()) {
        (*cursor)->set_mode(Cursor::CURSOR_MODE_HIDDEN);
    }

    Controls controls = Controls::from_settings();
    global.controls = &controls;

    Renderer renderer;
    global.renderer = &renderer;
    renderer.init(platform.window->size());
    renderer.resize(platform.window->size());

    // load texture atlas + resources, re-init renderer
    TextureAtlas::get() = TextureAtlas(uvec2(8192));
    resources::load(
        platform.resources_path,
        renderer);
    renderer.init_with_resources();

    // create sound engine
    SoundEngine sound;
    global.sound = &sound;

    // create user interface root
    UIRoot ui_root;
    global.ui = &ui_root;
    ui_root.configure();

    // run initializers
    InitRegistry::instance().init();

    // initialize component managers
    Tiles::get().init();
    Items::get().init();

    // initialize game states
    StateGame game;
    global.game = &game;
    global.game->init();
    global.state = global.game;

    /* bgfx::setDebug(BGFX_DEBUG_STATS); */

    // main return value
    int retval = 0;

    // divert to alternate main if requested by arguments
    if (argc >= 2 && !std::strcmp(argv[1], "--main")) {
        ASSERT(argc >= 3, "expected argument after --main");

        const auto &em = EntryManager::instance();
        ASSERT(em.contains(argv[2]), "No such main {}", argv[2]);

        LOG("Diverting to alternate main {}", argv[2]);
        retval = em[argv[2]](argc, argv);
        goto done;
    }

    while (!global.platform->window->is_close_requested()) {
        global.time->section_frame.begin();
        bgfx::dbgTextClear();

        global.platform->window->prepare_frame();
        renderer.prepare_frame();

        // handle window size changes
        if (global.platform->window->resized()) {
            renderer.resize(
                global.platform->window->size());

            // reposition UI elements after size change
            global.ui->reposition();
        }

        global.time->update();

        global.time->section_update.begin();
        {
            global.update();
            global.state->update();
        }
        global.time->section_update.end();

        global.time->tick(
            [&]() {
                global.time->section_tick.begin();
                {
                    global.tick();
                    TextureAtlas::get().tick();
                    global.state->tick();
                    global.tick_allocator.clear();
                }
                global.time->section_tick.end();

                // print timing stats every second
                if (global.time->ticks >= TICKS_PER_SECOND
                        && (global.time->ticks % TICKS_PER_SECOND) == 0) {
                    LOG(
                        "{}/{} "
                        "[T {:.3f} | U {:.3f} | R {:.3f} | F {:.3f}]",
                        global.time->fps,
                        global.time->tps,
                        Time::to_millis(global.time->section_tick.avg()),
                        Time::to_millis(global.time->section_update.avg()),
                        Time::to_millis(global.time->section_render.avg()),
                        Time::to_millis(global.time->section_frame.avg()));
                }
            });

        global.time->section_render.begin();
        {
            global.state->render();
        }
        global.time->section_render.end();

        renderer.end_frame();
        global.platform->window->end_frame();

        global.frame_allocator.clear();
        global.long_allocator.frame();
        global.time->section_frame.end();
    }

done:
    // destroy game states
    global.game->destroy();

    LOG("Exiting normally");
    std::exit(retval);
    return retval;
}
