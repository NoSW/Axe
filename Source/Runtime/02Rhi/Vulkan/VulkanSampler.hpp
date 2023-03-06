#pragma once

#include "02Rhi/Rhi.hpp"

#define VK_NO_PROTOTYPES  // used for volk
#include <vulkan/vulkan.h>

namespace axe::rhi
{
class VulkanDevice;
class VulkanSampler : public Sampler
{
private:
    AXE_NON_COPYABLE(VulkanSampler);
    VulkanSampler(VulkanDevice* device) noexcept : _mpDevice(device) {}
    bool _create(SamplerDesc&) noexcept;
    bool _destroy() noexcept;
    friend class VulkanDevice;

private:
    VulkanDevice* const _mpDevice                        = nullptr;
    VkSampler _mpHandle                                  = VK_NULL_HANDLE;
    VkSamplerYcbcrConversion _mpVkSamplerYcbcrConversion = VK_NULL_HANDLE;
    VkSamplerYcbcrConversionInfo _mVkSamplerYcbcrConversionInfo{};
};

}  // namespace axe::rhi