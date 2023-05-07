#pragma once

#include "01Resource/Config.hpp"

#include <span>

namespace axe::resource
{

// we get shader byte code by filename( which must be unique in Source/Shaders/)
const std::span<u8> get_spv_byte_code(std::string_view filename) noexcept;

// we get shader byte code by filename( which must be unique in Source/Shaders/)
const std::span<u8> get_dxil_byte_code(std::string_view filename) noexcept;

}  // namespace axe::resource