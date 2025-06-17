vec3 a_position         : POSITION;
vec3 a_normal           : NORMAL;
vec2 a_texcoord0        : TEXCOORD0;
vec2 a_texcoord1        : TEXCOORD1;
uint a_color0           : COLOR0;

vec2 v_texcoord0        : TEXCOORD0 = vec2(0.0, 0.0);
vec2 v_texcoord1        : TEXCOORD1 = vec2(0.0, 0.0);
vec3 v_normal           : NORMAL = vec3(0.0);
vec3 v_position         : POSITION = vec3(0.0);
flat uint v_color0      : COLOR0 = 0;
