#pragma once
#include "02Rhi/Rhi.hpp"
#include "VulkanEnums.hxx"

namespace axe::rhi
{

class VulkanDevice;

class VulkanSemaphore final : public Semaphore
{
    friend class VulkanDevice;
    friend class VulkanQueue;
    friend class VulkanSwapChain;
    AXE_NON_COPYABLE(VulkanSemaphore);
    VulkanSemaphore(VulkanDevice* device) noexcept : _mpDevice(device) {}
    bool _create(const SemaphoreDesc&) noexcept;
    bool _destroy() noexcept;

public:
    VulkanSemaphore() noexcept;

    AXE_PUBLIC ~VulkanSemaphore() noexcept override { AXE_ASSERT(_mpHandle == VK_NULL_HANDLE); }

public:
    auto handle() noexcept { return _mpHandle; }

public:
    constexpr static VkObjectType getVkTypeId() noexcept { return VK_OBJECT_TYPE_SEMAPHORE; }

private:
    VulkanDevice* const _mpDevice = nullptr;
    VkSemaphore _mpHandle         = VK_NULL_HANDLE;
    u32 _mSignaled : 1            = 0;
    u32 _mPadA                    = 0;
    u64 _mPadB                    = 0;
    u64 _mPadC                    = 0;
};

}  // namespace axe::rhi