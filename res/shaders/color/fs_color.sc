$input v_color0, v_normal

#include "common.sc"

void main() {
    gl_FragData[0] = vec4(v_color0.rgba);
    gl_FragData[1] = vec4(v_normal, 0);
    gl_FragData[2] = vec4(LIGHT_MAX_VALUE, vec3(0.0));
}
