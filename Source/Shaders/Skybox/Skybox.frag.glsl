#version 460

#extension GL_GOOGLE_include_directive : require

#include "Utils.glsl"

layout(set = 0, binding = 1) uniform texture2D RightText;
layout(set = 0, binding = 2) uniform texture2D LeftText;
layout(set = 0, binding = 3) uniform texture2D TopText;
layout(set = 0, binding = 4) uniform texture2D BotText;
layout(set = 0, binding = 5) uniform texture2D FrontText;
layout(set = 0, binding = 6) uniform texture2D BackText;
layout(set = 0, binding = 7) uniform sampler uSampler0;

layout(location = 0) in vec4 in_texCoord;
layout(location = 0) out vec4 out_color;

void main()
{
    switch (uint(round(in_texCoord.w)))
    {
        case 1: out_color = texture(sampler2D(RightText, uSampler0), flipXY(in_texCoord.zy / 20.0 + 0.5)); break;
        case 2: out_color = texture(sampler2D(LeftText, uSampler0), flipY(in_texCoord.zy / 20.0 + 0.5)); break;
        case 3: out_color = texture(sampler2D(TopText, uSampler0), flipNull(in_texCoord.xz / 20.0 + 0.5)); break;
        case 4: out_color = texture(sampler2D(BotText, uSampler0), flipY(in_texCoord.xz / 20.0 + 0.5)); break;
        case 5: out_color = texture(sampler2D(FrontText, uSampler0), flipY(in_texCoord.xy / 20.0 + 0.5)); break;
        case 6: out_color = texture(sampler2D(BackText, uSampler0), flipXY(in_texCoord.xy / 20.0 + 0.5)); break;
        default: out_color = texture(sampler2D(TopText, uSampler0), flipNull(in_texCoord.xz / 20.0 + 0.5)); break;
    }
}
