
#include "02Rhi/Vulkan/VulkanQueue.hpp"
#include "02Rhi/Vulkan/VulkanSemaphore.hpp"
#include "02Rhi/Vulkan/VulkanFence.hpp"
#include "02Rhi/Vulkan/VulkanCmd.hpp"
#include "02Rhi/Vulkan/VulkanSwapChain.hpp"
#include <volk/volk.h>

namespace axe::rhi
{

void VulkanQueue::_findQueueFamilyIndex(VulkanDriver* pDriver, u32 nodeIndex, QueueType quType,
                                        VkQueueFamilyProperties2& outQuProps2, u8& outQuFamIndex, u8& outQuIndex) noexcept
{
    AXE_ASSERT(pDriver);
    AXE_ASSERT(pDriver->mGpuMode == GPU_MODE_LINKED || nodeIndex == 0);

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

    u32 quFamPropCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties2(pDriver->mpVkActiveGPU, &quFamPropCount, nullptr);
    std::vector<VkQueueFamilyProperties2> quFamProp2(quFamPropCount, {VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2});
    vkGetPhysicalDeviceQueueFamilyProperties2(pDriver->mpVkActiveGPU, &quFamPropCount, quFamProp2.data());

    u32 minQueueFlag = U32_MAX;

    // Try to find a dedicated queue of this type
    AXE_ASSERT(nodeIndex < pDriver->mUsedQueueCounts.size());
    AXE_ASSERT(nodeIndex < pDriver->mAvailableQueueCounts.size());
    for (u32 i = 0; i < quFamProp2.size(); ++i)
    {
        auto quFlags          = quFamProp2[i].queueFamilyProperties.queueFlags;
        bool hasGraphicsFlag  = (quFlags & VK_QUEUE_GRAPHICS_BIT);
        bool meetRequiredFlag = (quFlags & requiredFlagBit) != 0;
        bool noExtraFlags     = (quFlags & ~requiredFlagBit) == 0;

        bool hasUnused        = pDriver->mUsedQueueCounts[nodeIndex][quFlags] <
                         pDriver->mAvailableQueueCounts[nodeIndex][quFlags];

        AXE_ASSERT(quFlags < pDriver->mUsedQueueCounts[nodeIndex].size());
        AXE_ASSERT(quFlags < pDriver->mAvailableQueueCounts[nodeIndex].size());

        if (quType == QUEUE_TYPE_GRAPHICS && hasGraphicsFlag)
        {
            found      = true;
            quFamIndex = i;
            quIndex    = 0;  // always return same one if graphics queue
            break;
        }
        if ((meetRequiredFlag && noExtraFlags) && hasUnused)
        {
            found      = true;
            quFamIndex = i;
            quIndex    = pDriver->mUsedQueueCounts[nodeIndex][quFlags];
            break;
        }
    }

    // Try to find a non-dedicated queue of this type if NOT provide a dedicated one.
    if (!found)
    {
        for (u32 i = 0; i < quFamProp2.size(); ++i)
        {
            auto quFlags   = quFamProp2[i].queueFamilyProperties.queueFlags;
            bool hasUnused = pDriver->mUsedQueueCounts[nodeIndex][quFlags] <
                             pDriver->mAvailableQueueCounts[nodeIndex][quFlags];
            if ((quFlags & requiredFlagBit) && hasUnused)
            {
                found      = true;
                quFamIndex = i;
                quIndex    = pDriver->mUsedQueueCounts[nodeIndex][quFlags];
                AXE_DEBUG("Not found a dedicated queue of {}, using a no-dedicated one(Flags={})",
                          reflection::enum_name(quType), (u32)quFlags)
                break;
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
    outQuProps2   = quFamProp2[quFamIndex];
    outQuFamIndex = quFamIndex;
    outQuIndex    = quIndex;
}

bool VulkanQueue::_create(QueueDesc& desc) noexcept
{
    u32 nodeIndex = _mpDriver->mGpuMode == GPU_MODE_LINKED ? desc.mNodeIndex : 0;
    u8 quFamIndex = U8_MAX, quIndex = U8_MAX;
    VkQueueFamilyProperties2 quProps2{.sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2, .pNext = nullptr};
    VulkanQueue::_findQueueFamilyIndex(_mpDriver, nodeIndex, desc.mType, quProps2, quFamIndex, quIndex);

    const auto queueFlags = quProps2.queueFamilyProperties.queueFlags;
    AXE_ASSERT(nodeIndex < _mpDriver->mUsedQueueCounts.size());
    AXE_ASSERT(queueFlags < _mpDriver->mUsedQueueCounts[nodeIndex].size());
    ++_mpDriver->mUsedQueueCounts[nodeIndex][queueFlags];

    _mpSubmitMutex = /* TODO */ nullptr;
    _mFlags        = queueFlags;
    AXE_ASSERT(_mpDriver->mpVkActiveGPUProperties);
    _mTimestampPeriod    = _mpDriver->mpVkActiveGPUProperties->properties.limits.timestampPeriod;
    _mVkQueueFamilyIndex = quFamIndex;
    _mVkQueueIndex       = quIndex;
    _mGpuMode            = _mpDriver->mGpuMode;
    _mType               = desc.mType;
    _mNodeIndex          = desc.mNodeIndex;

    if (_mpDriver->mGpuMode == GPU_MODE_UNLINKED) { _mNodeIndex = _mpDriver->mUnlinkedDriverIndex; }

    vkGetDeviceQueue(_mpDriver->mpVkDevice, _mVkQueueFamilyIndex, _mVkQueueIndex, &_mpVkQueue);
    return _mpVkQueue != VK_NULL_HANDLE;
}

bool VulkanQueue::_destroy() noexcept
{
    AXE_ASSERT(_mpDriver);
    AXE_ASSERT(_mpVkQueue);
    AXE_ASSERT(_mpDriver->mGpuMode == GPU_MODE_LINKED || _mNodeIndex == 0);
    --_mpDriver->mUsedQueueCounts[_mNodeIndex][_mFlags];
    _mpVkQueue = VK_NULL_HANDLE;
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
        cmdBufs.push_back(p->_mVkCmdBuffer);
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
    auto* pVkFence = desc.mpSignalFence ? ((VulkanFence*)desc.mpSignalFence)->_mpVkFence : VK_NULL_HANDLE;
    auto result    = vkQueueSubmit(_mpVkQueue, 1, &submitInfo, pVkFence);
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
        .pSwapchains        = &(pSwapchain->_mpVkSwapChain),
        .pImageIndices      = &presentIndex,
        .pResults           = nullptr,
    };
    // add lock to make sure multiple threads dont use the same queue simultaneously
    auto qu     = (pSwapchain->_mpPresentQueue && pSwapchain->_mpPresentQueue->_mpVkQueue) ?
                      pSwapchain->_mpPresentQueue->_mpVkQueue :
                      _mpVkQueue;
    auto result = vkQueuePresentKHR(_mpVkQueue, &presentInfo);
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

void VulkanQueue::waitIdle() noexcept { vkQueueWaitIdle(_mpVkQueue); }

}  // namespace axe::rhi