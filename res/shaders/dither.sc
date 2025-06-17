CONST(uint DITHER[16]) = {
    0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5
};

vec3 dither(vec3 pos_w, vec3 c) {
    uvec3 upos_w = uvec3(pos_w / (1.0f / 16.0f));
    uint offset_d = (upos_w.x ^ (upos_w.y >> 1) ^ (upos_w.z << 3));
    vec3 offset_c =
        vec3(
            DITHER[(offset_d + 0) % 16],
            DITHER[(offset_d + 1) % 16],
            DITHER[(offset_d + 2) % 16]) * (1.0f / 256.0f);
    c += offset_c;
    return c;
}
