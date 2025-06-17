#include "input/controls.hpp"
#include "global.hpp"
#include "platform/platform.hpp"
#include "util/assert.hpp"
#include "util/string.hpp"
#include "util/toml.hpp"

CompoundInputButton Controls::from_string(const std::string &str) {
    ASSERT(str.find(':') != std::string::npos);
    const auto split = strings::split(str, ":");
    ASSERT(split.size() == 2);
    const auto &input = global.platform->get_input<Input>(split[0]);
    return CompoundInputButton(input[split[1]]);
}

Controls Controls::from_settings() {
    Controls result;
    const auto &settings = (*global.settings)["controls"];

    result.primary = from_string(*settings["primary"].value<std::string>());
    result.secondary = from_string(*settings["secondary"].value<std::string>());
    result.up = from_string(*settings["up"].value<std::string>());
    result.down = from_string(*settings["down"].value<std::string>());
    result.left = from_string(*settings["left"].value<std::string>());
    result.right = from_string(*settings["right"].value<std::string>());
    result.jump = from_string(*settings["jump"].value<std::string>());
    result.drop = from_string(*settings["drop"].value<std::string>());
    result.inventory = from_string(*settings["inventory"].value<std::string>());

    return result;
}
