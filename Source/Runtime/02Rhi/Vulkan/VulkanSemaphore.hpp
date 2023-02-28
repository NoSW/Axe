#pragma once
#include "02Rhi/Rhi.hpp"

#define VK_NO_PROTOTYPES  // used for volk
#include <vulkan/vulkan.h>

namespace axe::rhi
{

class VulkanDriver;

class VulkanSemaphore final : public Semaphore
{
    friend class VulkanDriver;
    friend class VulkanQueue;
    friend class VulkanSwapChain;
    AXE_NON_COPYABLE(VulkanSemaphore);
    VulkanSemaphore(VulkanDriver* driver) noexcept : _mpDriver(driver) {}
    bool _create(SemaphoreDesc&) noexcept;
    bool _destroy() noexcept;

public:
    ~VulkanSemaphore() noexcept override = default;

private:
    VulkanDriver* const _mpDriver = nullptr;
    VkSemaphore _mpVkSemaphore    = VK_NULL_HANDLE;
    u32 _mCurrentNodeIndex : 5    = 0;
    u32 _mSignaled         : 1    = 0;
    u32 _mPadA                    = 0;
    u64 _mPadB                    = 0;
    u64 _mPadC                    = 0;
};

}  // namespace axe::rhi