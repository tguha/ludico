#pragma once

#include "util/util.hpp"

namespace strings {
inline std::string replace(
    std::string_view str,
    std::string_view find,
    std::string_view replacement) {
    auto res = std::string(str);
    size_t pos = std::string::npos;
    while ((pos = res.find(find)) != std::string::npos) {
        res.replace(pos, find.length(), replacement);
    }
    return res;
}

inline std::string ltrim(std::string_view str) {
    auto result = std::string(str);
    while (!result.empty() && std::isspace(result[0])) {
        result = result.substr(1);
    }
    return result;
}

inline std::string rtrim(std::string_view str) {
    auto result = std::string(str);
    while (!result.empty() && std::isspace(result[result.size() - 1])) {
        result = result.substr(0, result.size() - 1);
    }
    return result;
}

inline std::string trim(std::string_view str) {
    return ltrim(rtrim(str));
}

inline std::string to_lower(const std::string &str) {
    auto result = str;
    std::transform(
        result.begin(), result.end(), result.begin(),
        [](unsigned char c){ return std::tolower(c); });
    return result;
}

inline std::vector<std::string> split(
    std::string_view s,
    std::string_view delim) {
    size_t pos_start = 0, pos_end, delim_len = delim.length();
    std::string token;
    std::vector<std::string> res;

    while ((pos_end = s.find(delim, pos_start)) != std::string::npos) {
        token = s.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back(token);
    }

    res.push_back(std::string(s.substr(pos_start)));
    return res;
}

enum DelimiterBehavior {
    DISCARD = 0,
    ATTACH_LEFT = 1,
    ATTACH_RIGHT = 2
};

// delim can contain multiple delimiters
inline std::vector<std::string> split_ndelim(
    const std::string &s,
    const std::string &delim,
    DelimiterBehavior delmimiter_behavior = DelimiterBehavior::DISCARD) {
    size_t pos_start = 0, pos_end, delim_start, delim_end, delim_offset = 0;
    std::vector<std::string> res;

    while (
        (pos_end = s.find_first_of(delim, pos_start + delim_offset))
            != std::string::npos) {
        delim_start = pos_end;
        delim_end = pos_end;

        // keep eating delimiters
        while (delim.find(s[delim_end]) != std::string::npos) {
            delim_end++;
        }

        size_t token_end, next_start;

        // pick according to desired delimiter behavior
        delim_offset = 0;
        switch (delmimiter_behavior) {
            case DISCARD:
                token_end = pos_end;
                next_start = delim_end;
                break;
            case ATTACH_LEFT:
                token_end = delim_end;
                next_start = delim_end;
                break;
            case ATTACH_RIGHT:
                token_end = pos_end;
                next_start = delim_start;
                delim_offset = delim_end - delim_start;
                break;
        }

        const auto token = s.substr(pos_start, token_end - pos_start);
        pos_start = next_start;
        res.push_back(token);
    }

    res.push_back(s.substr(pos_start));
    return res;
}
}
