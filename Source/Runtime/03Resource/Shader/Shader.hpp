#pragma once

#include "03Resource/Config.hpp"
#include "02Rhi/Rhi.hpp"

#include <span>

namespace axe::resource
{
// request built in shader byte code by reletive filename( which must be unique in Source/Shaders/,, e.g., "Shaders/Basic.vert.glsl")
rhi::ShaderStageDesc get_shader_stage(std::string_view filename) noexcept;

}  // namespace axe::resource