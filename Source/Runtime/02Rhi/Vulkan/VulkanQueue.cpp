
#include "02Rhi/Vulkan/VulkanQueue.hpp"
#include "02Rhi/Vulkan/VulkanSemaphore.hpp"
#include "02Rhi/Vulkan/VulkanFence.hpp"
#include "02Rhi/Vulkan/VulkanCmd.hpp"
#include "02Rhi/Vulkan/VulkanSwapChain.hpp"
#include "02Rhi/Vulkan/VulkanDevice.hpp"
#include <volk/volk.h>

namespace axe::rhi
{

bool VulkanQueue::_create(const QueueDesc& desc) noexcept
{
    u8 quFamIndex = U8_MAX, quIndex = U8_MAX, quFlag = 0;
    _mpDevice->requestQueueIndex(desc.mType, quFamIndex, quIndex, quFlag);

    _mpSubmitMutex       = /* TODO */ nullptr;
    _mTimestampPeriod    = _mpDevice->_mpAdapter->requestGPUSettings().mTimestampPeriod;
    _mVkQueueFamilyIndex = quFamIndex;
    _mVkQueueIndex       = quIndex;
    _mType               = desc.mType;
    _mFlags              = quFlag;

    vkGetDeviceQueue(_mpDevice->handle(), _mVkQueueFamilyIndex, _mVkQueueIndex, &_mpHandle);
    return _mpHandle != VK_NULL_HANDLE;
}

bool VulkanQueue::_destroy() noexcept
{
    AXE_ASSERT(_mpDevice);
    AXE_ASSERT(_mpHandle);

    --_mpDevice->_mQueueInfos[_mFlags].mUsedCount;
    _mpHandle = VK_NULL_HANDLE;
    return true;
}

void VulkanQueue::submit(QueueSubmitDesc& desc) noexcept
{
    AXE_ASSERT(!desc.mCmds.empty());

    std::vector<VkSemaphore> waitVkSemaphores;
    for (const auto* item : desc.mWaitSemaphores)
    {
        auto* p = (VulkanSemaphore*)item;
        if (p->_mSignaled)
        {
            p->_mSignaled = false;
            waitVkSemaphores.push_back(p->handle());
        }
    }
    std::vector<VkPipelineStageFlags> waitMasks(waitVkSemaphores.size(), VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

    std::vector<VkSemaphore> signalVkSemaphores;
    for (const auto* item : desc.mSignalSemaphores)
    {
        auto* p = (VulkanSemaphore*)item;
        if (!p->_mSignaled)
        {
            p->_mSignaled = true;
            signalVkSemaphores.push_back(p->handle());
        }
    }

    std::vector<VkCommandBuffer> cmdBufs;
    for (const auto* item : desc.mCmds)
    {
        auto* p = (VulkanCmd*)item;
        cmdBufs.push_back(p->handle());
    }

    VkSubmitInfo submitInfo{
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext                = nullptr,
        .waitSemaphoreCount   = (u32)waitVkSemaphores.size(),
        .pWaitSemaphores      = waitVkSemaphores.data(),
        .pWaitDstStageMask    = waitMasks.data(),
        .commandBufferCount   = (u32)cmdBufs.size(),
        .pCommandBuffers      = cmdBufs.data(),
        .signalSemaphoreCount = (u32)signalVkSemaphores.size(),
        .pSignalSemaphores    = signalVkSemaphores.data(),
    };

    //  TODO: add lock to make sure multiple threads dont use the same queue simultaneously
    // Many setups have just one queue family and one queue. In this case, async compute, async transfer doesn't exist and we end up using
    // the same queue for all three operations
    auto* pFence = desc.mpSignalFence ? ((VulkanFence*)desc.mpSignalFence) : nullptr;
    auto result  = vkQueueSubmit(_mpHandle, 1, &submitInfo, pFence ? pFence->handle() : VK_NULL_HANDLE);
    if (VK_FAILED(result))
    {
        AXE_ERROR("Failed to submit queue due to {}", string_VkResult(result));
    }
    if (pFence)
    {
        pFence->_mSubmitted = true;
    }
}

void VulkanQueue::present(QueuePresentDesc& desc) noexcept
{
    AXE_ASSERT(desc.mpSwapChain);
    auto* pSwapchain = static_cast<VulkanSwapChain*>(desc.mpSwapChain);
    std::pmr::vector<VkSemaphore> signaledVkSemaphores;
    for (const auto* item : desc.mWaitSemaphores)
    {
        auto* p = (VulkanSemaphore*)(item);
        if (p->_mSignaled)
        {
            p->_mSignaled = false;
            signaledVkSemaphores.push_back(p->handle());
        }
    }

    u32 presentIndex = desc.mIndex;
    VkPresentInfoKHR presentInfo{
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext              = nullptr,
        .waitSemaphoreCount = (u32)signaledVkSemaphores.size(),
        .pWaitSemaphores    = signaledVkSemaphores.data(),
        .swapchainCount     = 1,
        .pSwapchains        = &(pSwapchain->_mpHandle),
        .pImageIndices      = &presentIndex,
        .pResults           = nullptr,
    };

    // add lock to make sure multiple threads dont use the same queue simultaneously
    auto result = vkQueuePresentKHR((pSwapchain->_mpPresentQueueHandle != VK_NULL_HANDLE ? pSwapchain->_mpPresentQueueHandle : _mpHandle), &presentInfo);
    switch (result)
    {
        case VK_SUCCESS:
        case VK_SUBOPTIMAL_KHR: break;
        case VK_ERROR_DEVICE_LOST:      // reset
        case VK_ERROR_OUT_OF_DATE_KHR:  // Fix bug where we get this error if window is closed before able to present queue.
        default:
            AXE_ERROR("Failed to present queue, {}", string_VkResult(result));
            break;
    }
}

void VulkanQueue::waitIdle() noexcept { vkQueueWaitIdle(_mpHandle); }

}  // namespace axe::rhi