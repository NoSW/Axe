#version 460

layout(location = 0) in vec4 in_position;
layout(location = 0) out vec4 out_texCoord;

void main()
{
    out_texCoord = in_position;
}