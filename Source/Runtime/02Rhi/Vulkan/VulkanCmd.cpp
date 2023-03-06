#include "02Rhi/Vulkan/VulkanCmd.hpp"
#include "02Rhi/Vulkan/VulkanCmdPool.hpp"
#include "02Rhi/Vulkan/VulkanQueue.hpp"

#include "02Rhi/Vulkan/VulkanDevice.hpp"

#include <volk/volk.h>

namespace axe::rhi
{

bool VulkanCmd::_create(CmdDesc& desc) noexcept
{
    _mpCmdPool       = (VulkanCmdPool*)desc.mpCmdPool;
    _mpQueue         = _mpCmdPool->_mpQueue;
    _mType           = _mpQueue->_mType;
    _mNodeIndex      = _mpQueue->_mNodeIndex;
    _mCmdBufferCount = desc.mCmdCount;

    VkCommandBufferAllocateInfo allocInfo{
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext              = nullptr,
        .commandPool        = _mpCmdPool->_mpHandle,
        .level              = desc.mSecondary ? VK_COMMAND_BUFFER_LEVEL_SECONDARY : VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = _mCmdBufferCount,
    };
    auto result = vkAllocateCommandBuffers(_mpDevice->_mpHandle, &allocInfo, &_mpHandle);
    if (VK_FAILED(result)) { AXE_ERROR("Failed to create VulkanCmd due to {}", string_VkResult(result)); }
    return _mpHandle != VK_NULL_HANDLE;
}

bool VulkanCmd::_destroy() noexcept
{
    AXE_ASSERT(_mpDevice && _mpDevice->_mpHandle && _mpHandle);
    // vkDeviceWaitIdle(mpDevice->getDevice());
    vkFreeCommandBuffers(_mpDevice->_mpHandle, _mpCmdPool->_mpHandle, _mCmdBufferCount, &_mpHandle);
    _mpHandle = VK_NULL_HANDLE;
    return true;
}

void VulkanCmd::begin() noexcept
{
    VkCommandBufferBeginInfo beginInfo{
        .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext            = nullptr,
        .flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr,
    };

    VkDeviceGroupCommandBufferBeginInfo deviceGroupBeginInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_GROUP_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,

    };
    if (false /*mGpuMode == GPU_MODE_LINKED*/)
    {
        deviceGroupBeginInfo.deviceMask = (1 << _mNodeIndex);
        beginInfo.pNext                 = &deviceGroupBeginInfo;
    }
    auto result = vkBeginCommandBuffer(_mpHandle, &beginInfo);
    if (VK_FAILED(result)) { AXE_ERROR("Failed to begin VulkanCmd due to {}", string_VkResult(result)); }

    // reset CPU side data
    _mpBoundPipelineLayout = nullptr;
}

void VulkanCmd::end() noexcept
{
    if (_mpVkActiveRenderPass) { vkCmdEndRenderPass(_mpHandle); }
    _mpVkActiveRenderPass = VK_NULL_HANDLE;
    auto result           = vkEndCommandBuffer(_mpHandle);
    if (VK_FAILED(result)) { AXE_ERROR("Failed to begin VulkanCmd due to {}", string_VkResult(result)); }
}
void VulkanCmd::bindRenderTargets() noexcept {}

void VulkanCmd::setViewport(float x, float y, float width, float height, float minDepth, float maxDepth) noexcept
{
    AXE_ASSERT(width > 0.0 && height > 0.0);
    VkViewport viewport{
        .x        = x,
        .y        = y + height,
        .width    = width,
        .height   = -height,
        .minDepth = minDepth,
        .maxDepth = maxDepth,
    };
    vkCmdSetViewport(_mpHandle, 0, 1, &viewport);
}

void VulkanCmd::setScissor(u32 x, u32 y, u32 width, u32 height) noexcept
{
    VkRect2D rect{
        .offset = {.x = (i32)x, .y = (i32)y},
        .extent = {.width = width, .height = height},
    };
    vkCmdSetScissor(_mpHandle, 0, 1, &rect);
}

void VulkanCmd::setStencilReferenceValue(u32 val) noexcept {}

void VulkanCmd::bindDescriptorSet() noexcept {}

void VulkanCmd::bindPushConstants() noexcept {}

void VulkanCmd::bindPipeline() noexcept {}

void VulkanCmd::bindIndexBuffer() noexcept {}

void VulkanCmd::bindVertexBuffer() noexcept {}

void VulkanCmd::draw(u32 vertexCount, u32 firstIndex) noexcept {}

void VulkanCmd::drawInstanced(u32 vertexCount, u32 firstIndex, u32 instanceCount, u32 firstInstance) noexcept {}

void VulkanCmd::drawIndexed(u32 indexCount, u32 firstIndex, u32 firstVertex) noexcept {}

void VulkanCmd::drawIndexedInstanced(u32 indexCount, u32 firstIndex, u32 instanceCount, u32 firstInstance, u32 firstVertex) noexcept {}

void VulkanCmd::dispatch(u32 groupCountX, u32 groupCountY, u32 groupCountZ) noexcept {}

void VulkanCmd::resourceBarrier() noexcept {}

void VulkanCmd::updateBuffer() noexcept {}

void VulkanCmd::updateSubresource() noexcept {}

void VulkanCmd::copySubresource() noexcept {}

void VulkanCmd::resetQueryPool() noexcept {}

void VulkanCmd::beginQuery() noexcept {}

void VulkanCmd::endQuery() noexcept {}

void VulkanCmd::resolveQuery() noexcept {}

void VulkanCmd::addDebugMarker() noexcept {}

void VulkanCmd::beginDebugMarker() noexcept {}

void VulkanCmd::writeDebugMarker() noexcept {}

void VulkanCmd::endDebugMarker() noexcept {}

}  // namespace axe::rhi