#pragma once
#include "02Rhi/Rhi.hpp"

#define VK_NO_PROTOTYPES  // used for volk
#include <vulkan/vulkan.h>

namespace axe::rhi
{

VkFilter to_vk(FilterType) noexcept;
VkSamplerMipmapMode to_vk(MipMapMode) noexcept;
VkSamplerAddressMode to_vk(AddressMode) noexcept;
VkCompareOp to_vk(CompareMode) noexcept;

}  // namespace axe::rhi