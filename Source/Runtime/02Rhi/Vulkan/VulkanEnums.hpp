#pragma once

#include "02Rhi/RhiEnums.hpp"

#define VK_NO_PROTOTYPES  // used for volk
#include <vulkan/vulkan.h>

#include <vulkan/vk_enum_string_helper.h>

namespace axe::rhi
{
inline constexpr u32 VULKAN_MAX_DESCRIPTOR_POOL_SIZE_ARRAY_COUNT = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT + 1;
inline constexpr u32 VULKAN_MAX_PLANE_COUNT                      = 3;

VkFilter to_vk(FilterType) noexcept;
VkSamplerMipmapMode to_vk(MipMapMode) noexcept;
VkSamplerAddressMode to_vk(AddressMode) noexcept;
VkCompareOp to_vk(CompareMode) noexcept;

}  // namespace axe::rhi