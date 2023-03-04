
#include "02Rhi/Vulkan/VulkanQueue.hpp"
#include "02Rhi/Vulkan/VulkanSemaphore.hpp"
#include "02Rhi/Vulkan/VulkanFence.hpp"
#include "02Rhi/Vulkan/VulkanCmd.hpp"
#include "02Rhi/Vulkan/VulkanSwapChain.hpp"
#include "02Rhi/Vulkan/VulkanDevice.hpp"
#include <volk/volk.h>

namespace axe::rhi
{

void VulkanQueue::_findQueueFamilyIndex(VulkanDevice* pDevice, u32 nodeIndex, QueueType quType,
                                        u8& outQuFamIndex, u8& outQuIndex, u8& outFlag) noexcept
{
    AXE_ASSERT(pDevice);
    // AXE_ASSERT(pDevice->mGpuMode == GPU_MODE_LINKED || nodeIndex == 0);

    bool found    = false;
    u8 quFamIndex = U8_MAX, quIndex = U8_MAX;
    auto requiredFlagBit = VK_QUEUE_FLAG_BITS_MAX_ENUM;
    switch (quType)
    {
        case QUEUE_TYPE_GRAPHICS: requiredFlagBit = VK_QUEUE_GRAPHICS_BIT; break;
        case QUEUE_TYPE_TRANSFER: requiredFlagBit = VK_QUEUE_TRANSFER_BIT; break;
        case QUEUE_TYPE_COMPUTE: requiredFlagBit = VK_QUEUE_COMPUTE_BIT; break;
        default: AXE_ERROR("Unsupported queue type {}", reflection::enum_name(quType)); break;
    }

    u32 minQueueFlag         = U32_MAX;

    // Try to find a dedicated queue of this type
    // AXE_ASSERT(nodeIndex < pDevice->mUsedQueueCounts.size());
    // AXE_ASSERT(nodeIndex < pDevice->mAvailableQueueCounts.size());
    auto& dedicatedQueueInfo = pDevice->_mQueueInfos[requiredFlagBit];
    if (dedicatedQueueInfo.mUsedCount < dedicatedQueueInfo.mAvailableCount)
    {
        found      = true;
        quFamIndex = dedicatedQueueInfo.mFamilyIndex;
        outFlag    = requiredFlagBit;
        found      = true;
        if (requiredFlagBit & VK_QUEUE_GRAPHICS_BIT)
        {
            quIndex = 0;  // always return same one if graphics queue
        }
        else
        {
            quIndex = dedicatedQueueInfo.mUsedCount++;
        }
    }
    else
    {
        // Try to find a non-dedicated queue of this type if NOT provide a dedicated one.
        for (u32 flag = 0; flag < pDevice->_mQueueInfos.size(); ++flag)
        {
            auto& info = pDevice->_mQueueInfos[flag];
            if (info.mFamilyIndex == 0) { outFlag = flag; }  // default flag
            if ((requiredFlagBit & flag) == requiredFlagBit)
            {
                if (info.mUsedCount < info.mAvailableCount)
                {
                    quFamIndex = info.mFamilyIndex;
                    quIndex    = dedicatedQueueInfo.mUsedCount++;
                    outFlag    = flag;
                    found      = true;
                    AXE_DEBUG("Not found a dedicated queue of {}, using a no-dedicated one(Flags={})",
                              reflection::enum_name(quType), flag)
                    break;
                }
            }
        }
    }

    // Choose default queue if all tries fail.
    if (!found)
    {
        found      = true;
        quFamIndex = 0;
        quIndex    = 0;
        AXE_DEBUG("Not found queue of {}, using default one(familyIndex=0.queueIndex=0)", reflection::enum_name(quType));
    }
    outQuFamIndex = quFamIndex;
    outQuIndex    = quIndex;
}

bool VulkanQueue::_create(QueueDesc& desc) noexcept
{
    u32 nodeIndex = 0;  //_mpDevice->mGpuMode == GPU_MODE_LINKED ? desc.mNodeIndex : 0;
    u8 quFamIndex = U8_MAX, quIndex = U8_MAX, quFlag = 0;
    VulkanQueue::_findQueueFamilyIndex(_mpDevice, nodeIndex, desc.mType, quFamIndex, quIndex, quFlag);

    _mpSubmitMutex       = /* TODO */ nullptr;
    _mTimestampPeriod    = _mpDevice->_mpAdapter->requestGPUSettings().mTimestampPeriod;
    _mVkQueueFamilyIndex = quFamIndex;
    _mVkQueueIndex       = quIndex;
    _mGpuMode            = GPU_MODE_SINGLE;  // TODO
    _mType               = desc.mType;
    _mNodeIndex          = desc.mNodeIndex;
    _mFlags              = quFlag;

    // if (_mpDevice->mGpuMode == GPU_MODE_UNLINKED) { _mNodeIndex = _mpDevice->mUnlinkedDeviceIndex; }

    vkGetDeviceQueue(_mpDevice->_mpHandle, _mVkQueueFamilyIndex, _mVkQueueIndex, &_mpHandle);
    return _mpHandle != VK_NULL_HANDLE;
}

bool VulkanQueue::_destroy() noexcept
{
    AXE_ASSERT(_mpDevice);
    AXE_ASSERT(_mpHandle);
    //   AXE_ASSERT(_mpDevice->mGpuMode == GPU_MODE_LINKED || _mNodeIndex == 0); // TODO
    --_mpDevice->_mQueueInfos[_mFlags].mUsedCount;
    _mpHandle = VK_NULL_HANDLE;
    return true;
}

void VulkanQueue::submit(QueueSubmitDesc& desc) noexcept
{
    AXE_ASSERT(!desc.mCmds.empty());

    std::vector<VkSemaphore> waitVkSemaphores;
    std::vector<u32> waitIndices;
    for (const auto* item : desc.mWaitSemaphores)
    {
        auto* p = (VulkanSemaphore*)item;
        if (p->_mSignaled)
        {
            p->_mSignaled = false;
            waitVkSemaphores.push_back(p->_mpVkSemaphore);
            waitIndices.push_back(p->_mCurrentNodeIndex);
        }
    }
    std::vector<VkPipelineStageFlags> waitMasks(waitVkSemaphores.size(), VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

    std::vector<VkSemaphore> signalVkSemaphores;
    std::vector<u32> signalIndices;
    for (const auto* item : desc.mSignalSemaphores)
    {
        auto* p = (VulkanSemaphore*)item;
        if (!p->_mSignaled)
        {
            p->_mSignaled         = true;
            p->_mCurrentNodeIndex = _mNodeIndex;
            signalVkSemaphores.push_back(p->_mpVkSemaphore);
            signalIndices.push_back(p->_mCurrentNodeIndex);
        }
    }

    std::vector<VkCommandBuffer> cmdBufs;
    std::vector<u32> deviceMasks;
    for (const auto* item : desc.mCmds)
    {
        auto* p = (VulkanCmd*)item;
        cmdBufs.push_back(p->_mpHandle);
        deviceMasks.push_back(1 << p->_mNodeIndex);
    }

    VkDeviceGroupSubmitInfo deviceGroupSubmitInfo{
        .sType                         = VK_STRUCTURE_TYPE_DEVICE_GROUP_SUBMIT_INFO_KHR,
        .pNext                         = nullptr,
        .waitSemaphoreCount            = (u32)waitIndices.size(),
        .pWaitSemaphoreDeviceIndices   = waitIndices.data(),
        .commandBufferCount            = (u32)deviceMasks.size(),
        .pCommandBufferDeviceMasks     = deviceMasks.data(),
        .signalSemaphoreCount          = (u32)signalIndices.size(),
        .pSignalSemaphoreDeviceIndices = signalIndices.data(),
    };

    VkSubmitInfo submitInfo{
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext                = _mGpuMode == GPU_MODE_UNLINKED ? &deviceGroupSubmitInfo : nullptr,
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
    auto* pVkFence = desc.mpSignalFence ? ((VulkanFence*)desc.mpSignalFence)->__mpHandle : VK_NULL_HANDLE;
    auto result    = vkQueueSubmit(_mpHandle, 1, &submitInfo, pVkFence);
    if (VK_FAILED(result)) { AXE_ERROR("Failed to submit queue due to {}", string_VkResult(result)); }
    if (pVkFence != VK_NULL_HANDLE) { ((VulkanFence*)desc.mpSignalFence)->_mSubmitted = true; }
}

void VulkanQueue::present(QueuePresentDesc& desc) noexcept
{
    AXE_ASSERT(desc.mpSwapChain);
    auto* pSwapchain = (VulkanSwapChain*)desc.mpSwapChain;
    std::pmr::vector<VkSemaphore> signaledVkSemaphores;
    for (const auto* item : desc.mWaitSemaphores)
    {
        auto* p = (VulkanSemaphore*)(item);
        if (p->_mSignaled)
        {
            p->_mSignaled = false;
            signaledVkSemaphores.push_back(p->_mpVkSemaphore);
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
    auto qu     = (pSwapchain->_mpPresentQueue && pSwapchain->_mpPresentQueue->_mpHandle) ?
                      pSwapchain->_mpPresentQueue->_mpHandle :
                      _mpHandle;
    auto result = vkQueuePresentKHR(_mpHandle, &presentInfo);
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