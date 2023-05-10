#version 460

#extension GL_GOOGLE_include_directive : require

#include "Config.glsl"
#include "Utils.glsl"

layout(set = UPDATE_FREQ_NONE, binding = 1) uniform texture2D RightText;
layout(set = UPDATE_FREQ_NONE, binding = 2) uniform texture2D LeftText;
layout(set = UPDATE_FREQ_NONE, binding = 3) uniform texture2D TopText;
layout(set = UPDATE_FREQ_NONE, binding = 4) uniform texture2D BotText;
layout(set = UPDATE_FREQ_NONE, binding = 5) uniform texture2D FrontText;
layout(set = UPDATE_FREQ_NONE, binding = 6) uniform texture2D BackText;
layout(set = UPDATE_FREQ_NONE, binding = 7) uniform sampler uSampler0;

layout(location = 0) in vec4 in_texCoord;
layout(location = 0) out vec4 out_color;

void main()
{
    switch (uint(round(in_texCoord.w)))
    {
        case 1: out_color = texture(sampler2D(RightText, uSampler0), flipXY(in_texCoord.zy / 20.0 + 0.5)); break;
        case 2: out_color = texture(sampler2D(LeftText, uSampler0),  flipY(in_texCoord.zy / 20.0 + 0.5)); break;
        case 3: out_color = texture(sampler2D(TopText, uSampler0),   flipNull(in_texCoord.xz / 20.0 + 0.5)); break;
        case 4: out_color = texture(sampler2D(BotText, uSampler0),   flipY(in_texCoord.xz / 20.0 + 0.5)); break;
        case 5: out_color = texture(sampler2D(FrontText, uSampler0), flipY(in_texCoord.xy / 20.0 + 0.5)); break;
        case 6: out_color = texture(sampler2D(BackText, uSampler0),  flipXY(in_texCoord.xy / 20.0 + 0.5)); break;
        default: out_color = texture(sampler2D(TopText, uSampler0),  flipNull(in_texCoord.xz / 20.0 + 0.5)); break;
    }
}
