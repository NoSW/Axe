#pragma once
#include "02Rhi/Vulkan/VulkanDriver.hpp"
#include "02Rhi/Vulkan/VulkanQueue.hpp"
#include "02Rhi/Vulkan/VulkanRenderTarget.hpp"

namespace axe::rhi
{

class VulkanSwapChain final : public SwapChain
{
    friend class VulkanDriver;
    friend class VulkanQueue;
    AXE_NON_COPYABLE(VulkanSwapChain);
    VulkanSwapChain(VulkanDriver* driver) noexcept : _mpDriver(driver) {}
    bool _create(SwapChainDesc&) noexcept;
    bool _destroy() noexcept;

public:
    ~VulkanSwapChain() noexcept override = default;

    void acquireNextImage(Semaphore* pSignalSemaphore, u32& outImageIndex) noexcept override;
    void acquireNextImage(Fence* pFence, u32& outImageIndex) noexcept override;

private:
    std::vector<VulkanRenderTarget*> _mpRenderTargets;  // created from the swapchain back buffers
    VulkanDriver* const _mpDriver = nullptr;
    /// Present queue if one exists (queuePresent will use this queue if the hardware has a dedicated present queue)
    VulkanQueue* _mpPresentQueue  = nullptr;
    VkSwapchainKHR _mpVkSwapChain = VK_NULL_HANDLE;
    VkSurfaceKHR _mpVkSurface     = VK_NULL_HANDLE;
    SwapChainDesc* _mpDesc        = nullptr;
    VkFormat _mVkSwapchainFormat  = VK_FORMAT_UNDEFINED;
    u32 _mPresentQueueFamilyIndex = U32_MAX;
    u32 _mPadA                    = 0;
    u32 _mImageCount  : 3         = 0;
    u32 _mEnableVsync : 1         = 0;
};

}  // namespace axe::rhi