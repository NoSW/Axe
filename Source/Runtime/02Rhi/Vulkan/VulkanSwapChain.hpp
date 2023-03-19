#pragma once

#include "02Rhi/Rhi.hpp"
#include "02Rhi/Vulkan/VulkanEnums.hpp"

namespace axe::rhi
{
class VulkanQueue;
class VulkanDevice;
class VulkanRenderTarget;

class VulkanSwapChain final : public SwapChain
{
    friend class VulkanDevice;
    friend class VulkanQueue;
    AXE_NON_COPYABLE(VulkanSwapChain);
    VulkanSwapChain(VulkanDevice* device) noexcept : _mpDevice(device) {}
    bool _create(SwapChainDesc&) noexcept;
    bool _destroy() noexcept;

public:
    AXE_PUBLIC ~VulkanSwapChain() noexcept override = default;
    AXE_PUBLIC void acquireNextImage(Semaphore* pSignalSemaphore, u32& outImageIndex) noexcept override;
    AXE_PUBLIC void acquireNextImage(Fence* pFence, u32& outImageIndex) noexcept override;

public:
    auto handle() noexcept { return _mpHandle; }

public:
    constexpr static VkObjectType TYPE_ID = VK_OBJECT_TYPE_SWAPCHAIN_KHR;

private:
    std::pmr::vector<VulkanRenderTarget*> _mpRenderTargets;  // created from the swapchain back buffers
    VulkanDevice* const _mpDevice = nullptr;

    /// Present queue if one exists (queuePresent will use this queue if the hardware has a dedicated present queue)
    VkQueue _mpPresentQueueHandle = VK_NULL_HANDLE;
    VkSwapchainKHR _mpHandle      = VK_NULL_HANDLE;
    VkSurfaceKHR _mpVkSurface     = VK_NULL_HANDLE;
    VkFormat _mVkSwapchainFormat  = VK_FORMAT_UNDEFINED;
    u32 _mPresentQueueFamilyIndex = U32_MAX;
    u32 _mImageCount  : 3         = 0;
    u32 _mEnableVsync : 1         = 0;
};

}  // namespace axe::rhi