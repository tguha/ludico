$input v_color0, v_normal

#include "common.sc"

void main() {
    vec4 color = v_color0;

    if (color.a < EPSILON) {
        discard;
    }

    WRITE_GBUFFERS(
        color,
        v_normal,
        0,
        0,
        0,
        0);
}
