#include "02Rhi/Vulkan/VulkanCmd.hpp"
#include "02Rhi/Vulkan/VulkanCmdPool.hpp"
#include "02Rhi/Vulkan/VulkanQueue.hpp"
#include "02Rhi/Vulkan/VulkanTexture.hpp"
#include "02Rhi/Vulkan/VulkanBuffer.hpp"

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
        .commandPool        = _mpCmdPool->handle(),
        .level              = desc.mSecondary ? VK_COMMAND_BUFFER_LEVEL_SECONDARY : VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = _mCmdBufferCount,
    };
    auto result = vkAllocateCommandBuffers(_mpDevice->handle(), &allocInfo, &_mpHandle);
    if (VK_FAILED(result)) { AXE_ERROR("Failed to create VulkanCmd due to {}", string_VkResult(result)); }
    return _mpHandle != VK_NULL_HANDLE;
}

bool VulkanCmd::_destroy() noexcept
{
    AXE_ASSERT(_mpDevice && _mpDevice->handle() && _mpHandle);
    // vkDeviceWaitIdle(mpDevice->getDevice());
    vkFreeCommandBuffers(_mpDevice->handle(), _mpCmdPool->handle(), _mCmdBufferCount, &_mpHandle);
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

void VulkanCmd::resourceBarrier(
    std::pmr::vector<TextureBarrier>* pTextureBarriers,
    std::pmr::vector<BufferBarrier>* pBufferBarriers,
    std::pmr::vector<RenderTargetBarrier>* pRTBarriers) noexcept
{
    VkAccessFlags srcAccessFlags = 0;
    VkAccessFlags dstAccessFlags = 0;
    const u32 imageBarrierCount  = (pTextureBarriers ? pTextureBarriers->size() : 0) + (pRTBarriers ? pRTBarriers->size() : 0);
    std::pmr::vector<VkImageMemoryBarrier> imgMemBarriers(imageBarrierCount);

    const u32 bufferBarrierCount = pBufferBarriers ? pBufferBarriers->size() : 0;
    std::pmr::vector<VkBufferMemoryBarrier> bufMemBarriers(bufferBarrierCount);

    if (imgMemBarriers.size() == 0 && bufMemBarriers.size() == 0)
    {
        return;  // early exit
    }

    const auto findQueueFamIndexHelper = [this](const BarrierInfo& info) -> std::pair<u8, u8>
    {
        if (info.mAcquire && info.mCurrentState != RESOURCE_STATE_UNDEFINED)
        {
            return {_mpDevice->_mQueueFamilyIndexes[(u32)info.mQueueType], (u8)_mpQueue->_mVkQueueFamilyIndex};
        }
        else if (info.mRelease && info.mCurrentState != RESOURCE_STATE_UNDEFINED)
        {
            return {(u8)_mpQueue->_mVkQueueFamilyIndex, _mpDevice->_mQueueFamilyIndexes[(u32)info.mQueueType]};
        }
        else
        {
            return {VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED};
        }
    };

    if (pBufferBarriers)
    {
        for (u32 i = 0; i < pBufferBarriers->size(); ++i)
        {
            auto& pTrans                        = pBufferBarriers->at(i);
            auto* pBuffer                       = static_cast<VulkanBuffer*>(pTrans.pBuffer);
            auto& bufMemTrans                   = bufMemBarriers[i];
            // blocked bu VulkanBuffer

            const bool isBothUA                 = pTrans.mBarrierInfo.mCurrentState == RESOURCE_STATE_UNORDERED_ACCESS && pTrans.mBarrierInfo.mNewState == RESOURCE_STATE_UNORDERED_ACCESS;
            auto [srcQuFamIndex, dstQuFamIndex] = findQueueFamIndexHelper(pTrans.mBarrierInfo);

            bufMemTrans.sType                   = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            bufMemTrans.pNext                   = nullptr;
            bufMemTrans.srcAccessMask           = isBothUA ? VK_ACCESS_SHADER_WRITE_BIT : resource_state_to_access_flags(pTrans.mBarrierInfo.mCurrentState);
            bufMemTrans.dstAccessMask           = isBothUA ? (VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT) : resource_state_to_access_flags(pTrans.mBarrierInfo.mNewState);
            bufMemTrans.buffer                  = pBuffer->handle();
            bufMemTrans.size                    = VK_WHOLE_SIZE;
            bufMemTrans.offset                  = 0;
            bufMemTrans.srcQueueFamilyIndex     = srcQuFamIndex;
            bufMemTrans.dstQueueFamilyIndex     = dstQuFamIndex;

            srcAccessFlags |= bufMemTrans.srcAccessMask;
            dstAccessFlags |= bufMemTrans.dstAccessMask;
        }
    }

    const auto imageBarrierHelper = [&findQueueFamIndexHelper, &srcAccessFlags, &dstAccessFlags](
                                        const ImageBarrier& pTrans, VulkanTexture* pTexture, VkImageMemoryBarrier& imgMemTrans)
    {
        const bool isBothUA                         = pTrans.mBarrierInfo.mCurrentState == RESOURCE_STATE_UNORDERED_ACCESS && pTrans.mBarrierInfo.mNewState == RESOURCE_STATE_UNORDERED_ACCESS;
        auto [srcQuFamIndex, dstQuFamIndex]         = findQueueFamIndexHelper(pTrans.mBarrierInfo);
        imgMemTrans.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imgMemTrans.pNext                           = nullptr;
        imgMemTrans.srcAccessMask                   = isBothUA ? VK_ACCESS_SHADER_WRITE_BIT : resource_state_to_access_flags(pTrans.mBarrierInfo.mCurrentState);
        imgMemTrans.dstAccessMask                   = isBothUA ? (VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT) : resource_state_to_access_flags(pTrans.mBarrierInfo.mNewState);
        imgMemTrans.oldLayout                       = isBothUA ? VK_IMAGE_LAYOUT_GENERAL : resource_state_to_image_layout(pTrans.mBarrierInfo.mCurrentState);
        imgMemTrans.newLayout                       = isBothUA ? VK_IMAGE_LAYOUT_GENERAL : resource_state_to_image_layout(pTrans.mBarrierInfo.mNewState);
        imgMemTrans.image                           = pTexture->handle();
        imgMemTrans.subresourceRange.aspectMask     = (VkImageAspectFlags)pTexture->_mAspectMask;
        imgMemTrans.subresourceRange.baseMipLevel   = pTrans.mSubresourceBarrier ? pTrans.mMipLevel : 0;
        imgMemTrans.subresourceRange.levelCount     = pTrans.mSubresourceBarrier ? 1 : VK_REMAINING_MIP_LEVELS;
        imgMemTrans.subresourceRange.baseArrayLayer = pTrans.mSubresourceBarrier ? pTrans.mArrayLayer : 0;
        imgMemTrans.subresourceRange.layerCount     = pTrans.mSubresourceBarrier ? 1 : VK_REMAINING_ARRAY_LAYERS;
        imgMemTrans.srcQueueFamilyIndex             = srcQuFamIndex;
        imgMemTrans.dstQueueFamilyIndex             = dstQuFamIndex;
        srcAccessFlags |= imgMemTrans.srcAccessMask;
        dstAccessFlags |= imgMemTrans.dstAccessMask;
    };

    u32 imageBarrierIndex = 0;
    if (pTextureBarriers)
    {
        for (u32 i = 0; i < pTextureBarriers->size(); ++i)
        {
            auto& textureBarrier = pTextureBarriers->at(i);
            imageBarrierHelper(textureBarrier.mImageBarrier, static_cast<VulkanTexture*>(textureBarrier.mpTexture), imgMemBarriers[imageBarrierIndex++]);
        }
    }
    if (pRTBarriers)
    {
        for (u32 i = 0; i < pRTBarriers->size(); ++i)
        {
            auto& rtBarrier = pRTBarriers->at(i);
            imageBarrierHelper(rtBarrier.mImageBarrier, static_cast<VulkanRenderTarget*>(rtBarrier.mpRenderTarget)->_mpTexture, imgMemBarriers[imageBarrierIndex++]);
        }
    }

    auto srcStageMask = determine_pipeline_stage_flags({.mAccessFlags         = srcAccessFlags,
                                                        .mQueueType           = (QueueType)_mType,
                                                        .mIsSupportGeomShader = (bool)_mpDevice->_mpAdapter->requestGPUSettings().mGeometryShaderSupported,
                                                        .mIsSupportTeseShader = (bool)_mpDevice->_mpAdapter->requestGPUSettings().mTessellationSupported,
                                                        .mIsSupportRayTracing = (bool)_mpDevice->_mRaytracingSupported});

    auto dstStageMask = determine_pipeline_stage_flags({.mAccessFlags         = dstAccessFlags,
                                                        .mQueueType           = (QueueType)_mType,
                                                        .mIsSupportGeomShader = (bool)_mpDevice->_mpAdapter->requestGPUSettings().mGeometryShaderSupported,
                                                        .mIsSupportTeseShader = (bool)_mpDevice->_mpAdapter->requestGPUSettings().mTessellationSupported,
                                                        .mIsSupportRayTracing = (bool)_mpDevice->_mRaytracingSupported});

    vkCmdPipelineBarrier(_mpHandle, srcStageMask, dstStageMask, 0,
                         0, nullptr,
                         (u32)bufMemBarriers.size(), bufMemBarriers.data(),
                         (u32)imgMemBarriers.size(), imgMemBarriers.data());
}

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