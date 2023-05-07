vec4 white() { return vec4(1.0); }

vec2 flipX(in vec2 uv) { return vec2(1.0 - uv.x, uv.y); }
vec2 flipY(in vec2 uv) { return vec2(uv.x, 1.0 - uv.y); }
vec2 flipXY(in vec2 uv) { return 1.0 - uv; }
vec2 flipNull(in vec2 uv) { return uv.xy; }
