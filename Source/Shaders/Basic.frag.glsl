#version 460

#extension GL_GOOGLE_include_directive : require

#include "Config.glsl"

layout (location = 0) in vec3 in_positionWS;
layout (location = 1) in vec3 in_normalWS;
layout (location = 2) in vec3 in_tangentWS;
layout (location = 3) in vec2 in_uv0;
layout (location = 4) in vec2 in_uv1;
layout (location = 5) in vec4 in_color0;

layout (location = 0) out vec4 out_color;


layout (set = UPDATE_FREQ_PER_FRAME, binding = 0) uniform TransformInfo 
{
	mat4 projection;
	mat4 model;
	mat4 view;
	vec3 camPos;
} g_transformInfo;

void main()
{
    out_color = vec4(in_positionWS.xyz, 1.0);
}