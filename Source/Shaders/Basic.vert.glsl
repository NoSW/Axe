#version 460

#extension GL_GOOGLE_include_directive : require

#include "Utils.glsl"

layout(location = 0) out vec4 out_color;

void main()
{
    out_color = white();
}