#pragma once
#include "02Rhi/Rhi.hpp"

#define VK_NO_PROTOTYPES  // used for volk
#include <vulkan/vulkan.h>

namespace axe::rhi
{

class VulkanDevice;
class VulkanQueue;
class VulkanSwapChain;

class VulkanFence final : public Fence
{
    friend class VulkanDevice;
    friend class VulkanQueue;
    friend class VulkanSwapChain;
    AXE_NON_COPYABLE(VulkanFence);
    VulkanFence(VulkanDevice* device) noexcept : _mpDevice(device) {}
    bool _create(FenceDesc& desc) noexcept;
    bool _destroy() noexcept;

public:
    ~VulkanFence() noexcept override = default;
    void wait() noexcept override;
    FenceStatus status() noexcept override;

private:
    VulkanDevice* const _mpDevice = nullptr;
    VkFence __mpHandle            = VK_NULL_HANDLE;
    u32 _mSubmitted : 1           = 0;
    u32 _mPadA                    = 0;
    u64 _mPadB                    = 0;
    u64 _mPadC                    = 0;
};

}  // namespace axe::rhi