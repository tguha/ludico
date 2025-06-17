#pragma once

#include "util/types.hpp"
#include "util/math.hpp"
#include "util/util.hpp"

struct Path {
    std::vector<ivec3> points;

    inline usize size() const {
        return points.size();
    }

    inline auto &operator[](usize i) const { return this->points[i]; }
    inline auto &operator[](usize i) { return this->points[i]; }

    auto begin() const { return points.begin(); }
    auto end() const { return points.end(); }
    auto begin() { return points.begin(); }
    auto end() { return points.end(); }

    inline void add(const ivec3 &p) {
        this->points.push_back(p);
    }

    inline void reverse() {
        std::reverse(this->begin(), this->end());
    }

    std::string to_string() const {
        return fmt::format("Path{}", this->points);
    }
};
