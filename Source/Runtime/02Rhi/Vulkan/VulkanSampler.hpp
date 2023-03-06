#pragma once

#include "02Rhi/Rhi.hpp"
#include "02Rhi/Vulkan/VulkanEnums.hpp"

namespace axe::rhi
{
class VulkanDevice;
class VulkanSampler : public Sampler
{
private:
    AXE_NON_COPYABLE(VulkanSampler);
    VulkanSampler(VulkanDevice* device) noexcept : _mpDevice(device) {}
    ~VulkanSampler() noexcept override = default;
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