#pragma once

#include "util/util.hpp"
#include "util/types.hpp"
#include "util/result.hpp"

namespace file {
inline bool exists(const std::string &path) {
    return std::filesystem::exists(path);
}

inline result::Result<std::vector<std::string>, std::string>
    list_files(const std::string &path) {
    if (!std::filesystem::is_directory(path)) {
        return result::Err(path + " is not a directory");
    }

    std::vector<std::string> res;

    try {
        for (auto const &e : std::filesystem::directory_iterator(path)) {
            res.push_back(e.path().string());
        }
    } catch (const std::exception &e) {
        return result::Err(std::string(e.what()));
    }

    return result::Ok(res);
}

inline result::Result<bool, std::string> is_directory(
    const std::string &path) {
    if (std::filesystem::exists(path)) {
        return result::Ok(std::filesystem::is_directory(path));
    } else {
        return result::Err(path + " does not exist");
    }
}

inline result::Result<std::string, std::string> read_string(
    const std::string &path) {
    std::ifstream input(path);

    if (!input.good()) {
        return result::Err("Error opening file " + path);
    }

    std::stringstream buf;
    buf << input.rdbuf();

    if (input.fail()) {
        return result::Err("Error reading file " + path);
    }

    return result::Ok(buf.str());
}

inline result::Result<std::vector<u8>, std::string> read_file(
    const std::string &path) {
    std::ifstream input(path);

    if (!input.good()) {
        return result::Err("Error opening file " + path);
    }

    const auto result =
        std::vector<u8>(
            std::istreambuf_iterator<char>(input), {});

    if (input.fail()) {
        return result::Err("Error reading file " + path);
    }

    return result::Ok(result);
}

// TODO: doc, bulletproof
inline result::Result<void, std::string> write_file(
    const std::string &path,
    std::span<const u8> data) {
    std::ofstream out(path, std::ios::binary | std::ios::out);
    if (!out.is_open() || !out.good()) {
        return result::Err("error opening file " + path);
    }

    out.write(
        reinterpret_cast<char*>(
            const_cast<u8*>(&data[0])),
        data.size_bytes());

    if (out.fail()) {
        return result::Err("error writing file " + path);
    }

    out.close();
    return result::Ok();
}

// splits a path into { base, filename, ext }
inline std::tuple<std::string, std::string, std::string> split_path(
    const std::string &path) {
    auto last_sep = path.find_last_of("/\\"),
         ext_sep = path.find_last_of(".");

    const auto base =
        last_sep == std::string::npos ?
            ""
            : path.substr(0, last_sep);

    const auto filename =
        last_sep == std::string::npos ?
            path.substr(0, ext_sep)
            : path.substr(
                last_sep + 1,
                ext_sep == std::string::npos ?
                    std::string::npos
                    : (ext_sep - (last_sep + 1)));

    const auto ext =
        ext_sep == std::string::npos ?
            ""
            : path.substr(ext_sep + 1);

    return { base, filename, ext };
}
}
