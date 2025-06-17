#include "wfc/wfc.hpp"
#include "entry.hpp"
#include "gfx/util.hpp"
#include "gfx/image.hpp"
#include "global.hpp"
#include "util/rand.hpp"
#include "util/time.hpp"

int wfc_main(int argc, char **argv) {
    const auto [size_data, data] =
        gfx::load_texture_data("res/wfc_test.png")
            .unwrap();

    const auto size_result = uvec2(32, 32);

    for (usize i = 0; i < 5; i++) {
        global.time->time_section(
            "wfc",
            [&, size_data = size_data, data = data]() {
                Image out(size_result);
                auto w =
                    wfc::WFC<u32, 2, 3, 300>(
                        ivec2(size_data),
                        &reinterpret_cast<u32*>(data)[0],
                        wfc::PatternFunction::WEIGHTED,
                        wfc::NextCellFunction::MIN_ENTROPY,
                        wfc::BorderBehavior::CLAMP,
                        Rand(0x1234567 + i),
                        wfc::FLAG_ROTATE | wfc::FLAG_REFLECT);

                const auto [size_ts, data_ts] = w.dump_patterns();
                gfx::write_texture_data(
                    "tileset.png",
                    size_ts,
                    reinterpret_cast<const u8*>(&data_ts[0]));

                w.collapse(
                    ivec2(size_result),
                    &out[0]);

                out.scaled(uvec2(512, 512), Image::SCALE_NEAREST)
                    .write(fmt::format("output_{}.png", i), true);
            });
    }

    return 0;
}

DECL_ENTRY_POINT(wfc, wfc_main)
