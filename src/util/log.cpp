#include "util/log.hpp"
#include "platform/platform.hpp"
#include "global.hpp"

void log::out(
    Level level,
    const std::string &file,
    usize line,
    const std::string &fn,
    const std::string &s) {
    const auto level_name =
        ((std::string[]){ "NORMAL", "WARN", "ERROR", "DEBUG" })[level];
    std::string f = file;
    f = strings::replace(f, "src/", "");
    const auto res = fmt::vformat(
        std::string_view("[{}][{}:{}][{}] {}{}"),
        fmt::make_format_args(
            level_name, f, line, fn, s,
            s[s.length() - 1] == '\n' ? "" : "\n"));
    log::write(level, res);
}

void log::write(Level level, const std::string &s) {
    auto
        &serr = global.platform->log_err ? *global.platform->log_err : std::cerr,
        &sout = global.platform->log_out ? *global.platform->log_out : std::cout;
    (level == Level::ERROR ? serr : sout) << s;
}
