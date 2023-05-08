#pragma once

#include "02Rhi/Rhi.hpp"
#include "VulkanEnums.hxx"

namespace axe::rhi
{
class VulkanDevice;
class VulkanRootSignature;
class VulkanSampler : public Sampler
{
private:
    AXE_NON_COPYABLE(VulkanSampler);
    VulkanSampler(VulkanDevice* device) noexcept : _mpDevice(device) {}
    bool _create(const SamplerDesc&) noexcept;
    bool _destroy() noexcept;
    friend class VulkanDevice;
    friend class VulkanRootSignature;

public:
    AXE_PUBLIC ~VulkanSampler() noexcept override { AXE_ASSERT(_mpHandle == VK_NULL_HANDLE); }

public:
    auto handle() noexcept { return _mpHandle; }

public:
    constexpr static VkObjectType getVkTypeId() noexcept { return VK_OBJECT_TYPE_SAMPLER; }

private:
    VulkanDevice* const _mpDevice                        = nullptr;
    VkSampler _mpHandle                                  = VK_NULL_HANDLE;
    VkSamplerYcbcrConversion _mpVkSamplerYcbcrConversion = VK_NULL_HANDLE;
    VkSamplerYcbcrConversionInfo _mVkSamplerYcbcrConversionInfo{};
};

}  // namespace axe::rhi