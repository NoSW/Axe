#pragma once

#include "02Rhi/Rhi.hpp"
#include "VulkanEnums.internal.hpp"

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
    ~VulkanSwapChain() noexcept override = default;
    void acquireNextImage(Semaphore* pSignalSemaphore, u32& outImageIndex) noexcept override;
    void acquireNextImage(Fence* pFence, u32& outImageIndex) noexcept override;

public:
    AXE_PRIVATE auto handle() noexcept { return _mpHandle; }

    AXE_PRIVATE RenderTarget* getRenderTarget(u32 index) noexcept override
    {
        AXE_ASSERT(index < _mpRenderTargets.size());
        return _mpRenderTargets[index];
    }

public:
    AXE_PRIVATE constexpr static auto VK_TYPE_ID = VK_OBJECT_TYPE_SWAPCHAIN_KHR;

private:
    std::pmr::vector<RenderTarget*> _mpRenderTargets;  // created from the swapchain back buffers
    VulkanDevice* const _mpDevice = nullptr;

    /// Present queue if one exists (queuePresent will use this queue if the hardware has a dedicated present queue)
    VkQueue _mpPresentQueueHandle = VK_NULL_HANDLE;
    VkSwapchainKHR _mpHandle      = VK_NULL_HANDLE;
    VkSurfaceKHR _mpVkSurface     = VK_NULL_HANDLE;
    VkFormat _mVkSwapchainFormat  = VK_FORMAT_UNDEFINED;
    u32 _mPresentQueueFamilyIndex = U32_MAX;
    bool _mEnableVsync            = 0;
};

}  // namespace axe::rhi