#version 460

#extension GL_GOOGLE_include_directive : require

#include "Config.glsl"
#include "Utils.glsl"

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec3 in_tangent;
layout (location = 3) in vec2 in_uv0;
layout (location = 4) in vec2 in_uv1;
layout (location = 5) in vec4 in_color0;

layout (location = 0) out vec3 out_positionWS;
layout (location = 1) out vec3 out_normalWS;
layout (location = 2) out vec3 out_tangentWS;
layout (location = 3) out vec2 out_uv0;
layout (location = 4) out vec2 out_uv1;
layout (location = 5) out vec4 out_color0;


layout (set = UPDATE_FREQ_PER_FRAME, binding = 0) uniform TransformInfo 
{
	mat4 projection;
	mat4 model;
	mat4 view;
	vec3 camPos;
} g_transformInfo;

void main() 
{
	out_color0 = in_color0;

	vec4 positionWS = g_transformInfo.model * vec4(in_position, 1.0);
	out_normalWS = normalize(transpose(inverse(mat3(g_transformInfo.model))) * in_normal);
	
	positionWS.y = -positionWS.y;
	out_positionWS = positionWS.xyz / positionWS.w;
	out_uv0 = in_uv0;
	out_uv1 = in_uv1;

	gl_Position =  g_transformInfo.projection * g_transformInfo.view * vec4(out_positionWS, 1.0);
}
