#pragma once

#include "util/types.hpp"
#include "util/math.hpp"

namespace color {
    inline vec3 rgb_to_xyz(vec3 c) {
        vec3 tmp;
        tmp.x = (c.x > 0.04045) ? pow((c.x + 0.055) / 1.055, 2.4) : c.x / 12.92;
        tmp.y = (c.y > 0.04045) ? pow((c.y + 0.055) / 1.055, 2.4) : c.y / 12.92;
        tmp.z = (c.z > 0.04045) ? pow((c.z + 0.055) / 1.055, 2.4) : c.z / 12.92;
        const auto mat = mat3(
            { 0.4124, 0.3576, 0.1805 },
            { 0.2126, 0.7152, 0.0722 },
            { 0.0193, 0.1192, 0.9505 });
        return 100.0f * (mat * tmp);
    }

    inline vec3 xyz_to_lab(vec3 c) {
        const auto n = c / vec3(95.047, 100, 108.883);
        vec3 v;
        v.x = (n.x > 0.008856) ? pow(n.x, 1.0 / 3.0) : (7.787 * n.x) + (16.0 / 116.0);
        v.y = (n.y > 0.008856) ? pow(n.y, 1.0 / 3.0) : (7.787 * n.y) + (16.0 / 116.0);
        v.z = (n.z > 0.008856) ? pow(n.z, 1.0 / 3.0) : (7.787 * n.z) + (16.0 / 116.0);
        return vec3(
            (116.0 * v.y) - 16.0,
            500.0 * (v.x - v.y),
            200.0 * (v.y - v.z));
    }

    inline vec3 rgb_to_lab(vec3 c) {
        const auto lab = xyz_to_lab(rgb_to_xyz(c));
        return vec3(
            lab.x / 100.0,
            0.5 + 0.5 * (lab.y / 127.0),
            0.5 + 0.5 * (lab.z / 127.0));
    }

    inline vec3 lab_to_xyz(vec3 c) {
        f32 fy = (c.x + 16.0) / 116.0;
        f32 fx = c.y / 500.0 + fy;
        f32 fz = fy - c.z / 200.0;
        return vec3(
            95.047 * ((fx > 0.206897) ? fx * fx * fx : (fx - 16.0 / 116.0) / 7.787),
            100.000 * ((fy > 0.206897) ? fy * fy * fy : (fy - 16.0 / 116.0) / 7.787),
            108.883 * ((fz > 0.206897) ? fz * fz * fz : (fz - 16.0 / 116.0) / 7.787));
    }

    inline vec3 xyz_to_rgb(vec3 c) {
        const auto mat = mat3(
                { 3.2406, -1.5372, -0.4986 },
                { -0.9689, 1.8758, 0.0415 },
                { 0.0557, -0.2040, 1.0570 });
        const auto v = mat * (c * (1.0f / 100.0f));
        vec3 r;
        r.x = (v.x > 0.0031308) ? ((1.055 * pow(v.x, (1.0 / 2.4))) - 0.055) : 12.92 * v.x;
        r.y = (v.y > 0.0031308) ? ((1.055 * pow(v.y, (1.0 / 2.4))) - 0.055) : 12.92 * v.y;
        r.z = (v.z > 0.0031308) ? ((1.055 * pow(v.z, (1.0 / 2.4))) - 0.055) : 12.92 * v.z;
        return r;
    }

    inline vec3 lab_to_rgb(vec3 c) {
        return xyz_to_rgb(lab_to_xyz({100.0 * c.x, 2.0 * 127.0 * (c.y - 0.5), 2.0 * 127.0 * (c.z - 0.5)}));
    }

    inline vec4 rgba32_to_v(u32 c) {
        return vec4(
            ((c >>  0) & 0xFF) / 255.0f,
            ((c >>  8) & 0xFF) / 255.0f,
            ((c >> 16) & 0xFF) / 255.0f,
            ((c >> 24) & 0xFF) / 255.0f);
    }

    inline vec4 argb32_to_v(u32 c) {
        return vec4(
            ((c >> 16) & 0xFF) / 255.0f,
            ((c >>  8) & 0xFF) / 255.0f,
            ((c >>  0) & 0xFF) / 255.0f,
            ((c >> 24) & 0xFF) / 255.0f);
    }
}
