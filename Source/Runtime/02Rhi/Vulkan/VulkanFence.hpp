#pragma once
#include "02Rhi/Rhi.hpp"

#define VK_NO_PROTOTYPES  // used for volk
#include <vulkan/vulkan.h>

namespace axe::rhi
{

class VulkanDriver;
class VulkanQueue;
class VulkanSwapChain;

class VulkanFence final : public Fence
{
    friend class VulkanDriver;
    friend class VulkanQueue;
    friend class VulkanSwapChain;
    AXE_NON_COPYABLE(VulkanFence);
    VulkanFence(VulkanDriver* driver) noexcept : _mpDriver(driver) {}
    bool _create(FenceDesc& desc) noexcept;
    bool _destroy() noexcept;

public:
    ~VulkanFence() noexcept override = default;
    void wait() noexcept override;
    FenceStatus status() noexcept override;

private:
    VulkanDriver* const _mpDriver = nullptr;
    VkFence _mpVkFence            = VK_NULL_HANDLE;
    u32 _mSubmitted : 1           = 0;
    u32 _mPadA                    = 0;
    u64 _mPadB                    = 0;
    u64 _mPadC                    = 0;
};

}  // namespace axe::rhi