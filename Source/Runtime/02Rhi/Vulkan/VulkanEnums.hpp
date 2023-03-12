#pragma once

#include "02Rhi/RhiEnums.hpp"

#define VK_NO_PROTOTYPES  // used for volk
#include <vulkan/vulkan.h>

#include <vulkan/vk_enum_string_helper.h>

namespace axe::rhi
{

enum
{
    MAX_DESCRIPTOR_POOL_SIZE_ARRAY_COUNT = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT + 1,
    MAX_PLANE_COUNT                      = 3
};

// clang-format off
VkFilter             to_vk_enum(FilterType) noexcept;
VkSamplerMipmapMode  to_vk_enum(MipMapMode) noexcept;
VkSamplerAddressMode to_vk_enum(AddressMode) noexcept;
VkCompareOp          to_vk_enum(CompareMode) noexcept;
VkDescriptorType     to_vk_enum(DescriptorType) noexcept;
VkShaderStageFlags   to_vk_enum(ShaderStageFlag stages) noexcept;
// clang-format on

}  // namespace axe::rhi