#include "VulkanCmd.hxx"
#include "VulkanCmdPool.hxx"
#include "VulkanQueue.hxx"
#include "VulkanTexture.hxx"
#include "VulkanBuffer.hxx"
#include "VulkanDescriptorSet.hxx"
#include "VulkanDevice.hxx"

#include <volk.h>

namespace axe::rhi
{

bool VulkanCmd::_create(CmdDesc& desc) noexcept
{
    _mpCmdPool       = (VulkanCmdPool*)desc.pCmdPool;
    _mpQueue         = _mpCmdPool->_mpQueue;
    _mType           = _mpQueue->_mType;
    _mCmdBufferCount = desc.cmdCount;

    VkCommandBufferAllocateInfo allocInfo{
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext              = nullptr,
        .commandPool        = _mpCmdPool->handle(),
        .level              = desc.isSecondary ? VK_COMMAND_BUFFER_LEVEL_SECONDARY : VK_COMMAND_BUFFER_LEVEL_PRIMARY,
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

void VulkanCmd::bindDescriptorSet(u32 index, DescriptorSet* pSet) noexcept
{
    auto* pVulkanDescriptorSet    = (VulkanDescriptorSet*)pSet;
    auto* pVulkanRootSignature    = pVulkanDescriptorSet->_mpRootSignature;
    VkPipelineBindPoint bindPoint = to_pipeline_bind_point(pVulkanDescriptorSet->_mpRootSignature->_mPipelineType);

    AXE_ASSERT(index < pVulkanDescriptorSet->_mpHandles.size());
    if (_mpBoundPipelineLayout != pVulkanDescriptorSet->_mpRootSignature->_mpPipelineLayout)
    {
        _mpBoundPipelineLayout = pVulkanDescriptorSet->_mpRootSignature->_mpPipelineLayout;
        for (u32 i = 0; i < DESCRIPTOR_UPDATE_FREQ_COUNT; ++i)
        {
            if (pVulkanDescriptorSet->_mpRootSignature->_mpDescriptorSetLayouts[i] == VK_NULL_HANDLE)
            {
                vkCmdBindDescriptorSets(_mpHandle, bindPoint, _mpBoundPipelineLayout, i, 1, &_mpDevice->_mpEmptyDescriptorSet, 0, nullptr);
            }
        }
    }
    constexpr u32 VK_MAX_ROOT_DESCRIPTORS = 32;
    std::array<u32, VK_MAX_ROOT_DESCRIPTORS> offsets{};  // ?

    vkCmdBindDescriptorSets(
        _mpHandle, bindPoint, _mpBoundPipelineLayout,
        pVulkanDescriptorSet->_mUpdateFrequency, 1, &pVulkanDescriptorSet->_mpHandles[index],
        pVulkanDescriptorSet->_mDynamicUniformData.size(), offsets.data());
}

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
        if (info.isAcquire && info.currentState != RESOURCE_STATE_UNDEFINED)
        {
            return {_mpDevice->_mQueueFamilyIndexes[(u32)info.queueType], (u8)_mpQueue->_mVkQueueFamilyIndex};
        }
        else if (info.isRelease && info.currentState != RESOURCE_STATE_UNDEFINED)
        {
            return {(u8)_mpQueue->_mVkQueueFamilyIndex, _mpDevice->_mQueueFamilyIndexes[(u32)info.queueType]};
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

            const bool isBothUA                 = pTrans.barrierInfo.currentState == RESOURCE_STATE_UNORDERED_ACCESS && pTrans.barrierInfo.newState == RESOURCE_STATE_UNORDERED_ACCESS;
            auto [srcQuFamIndex, dstQuFamIndex] = findQueueFamIndexHelper(pTrans.barrierInfo);

            bufMemTrans.sType                   = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            bufMemTrans.pNext                   = nullptr;
            bufMemTrans.srcAccessMask           = isBothUA ? VK_ACCESS_SHADER_WRITE_BIT : resource_state_to_access_flags(pTrans.barrierInfo.currentState);
            bufMemTrans.dstAccessMask           = isBothUA ? (VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT) : resource_state_to_access_flags(pTrans.barrierInfo.newState);
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
        const bool isBothUA                         = pTrans.barrierInfo.currentState == RESOURCE_STATE_UNORDERED_ACCESS && pTrans.barrierInfo.newState == RESOURCE_STATE_UNORDERED_ACCESS;
        auto [srcQuFamIndex, dstQuFamIndex]         = findQueueFamIndexHelper(pTrans.barrierInfo);
        imgMemTrans.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imgMemTrans.pNext                           = nullptr;
        imgMemTrans.srcAccessMask                   = isBothUA ? VK_ACCESS_SHADER_WRITE_BIT : resource_state_to_access_flags(pTrans.barrierInfo.currentState);
        imgMemTrans.dstAccessMask                   = isBothUA ? (VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT) : resource_state_to_access_flags(pTrans.barrierInfo.newState);
        imgMemTrans.oldLayout                       = isBothUA ? VK_IMAGE_LAYOUT_GENERAL : resource_state_to_image_layout(pTrans.barrierInfo.currentState);
        imgMemTrans.newLayout                       = isBothUA ? VK_IMAGE_LAYOUT_GENERAL : resource_state_to_image_layout(pTrans.barrierInfo.newState);
        imgMemTrans.image                           = pTexture->handle();
        imgMemTrans.subresourceRange.aspectMask     = (VkImageAspectFlags)pTexture->_mAspectMask;
        imgMemTrans.subresourceRange.baseMipLevel   = pTrans.isSubresourceBarrier ? pTrans.mipLevel : 0;
        imgMemTrans.subresourceRange.levelCount     = pTrans.isSubresourceBarrier ? 1 : VK_REMAINING_MIP_LEVELS;
        imgMemTrans.subresourceRange.baseArrayLayer = pTrans.isSubresourceBarrier ? pTrans.arrayLayer : 0;
        imgMemTrans.subresourceRange.layerCount     = pTrans.isSubresourceBarrier ? 1 : VK_REMAINING_ARRAY_LAYERS;
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
            imageBarrierHelper(textureBarrier.imageBarrier, static_cast<VulkanTexture*>(textureBarrier.pTexture), imgMemBarriers[imageBarrierIndex++]);
        }
    }
    if (pRTBarriers)
    {
        for (u32 i = 0; i < pRTBarriers->size(); ++i)
        {
            auto& rtBarrier = pRTBarriers->at(i);
            imageBarrierHelper(rtBarrier.imageBarrier, static_cast<VulkanRenderTarget*>(rtBarrier.pRenderTarget)->_mpTexture, imgMemBarriers[imageBarrierIndex++]);
        }
    }

    auto srcStageMask = determine_pipeline_stage_flags({.mAccessFlags      = srcAccessFlags,
                                                        .queueType         = (QueueTypeFlag)_mType,
                                                        .supportGeomShader = (bool)_mpDevice->_mpAdapter->requestGPUSettings().geometryShaderSupported,
                                                        .supportTeseShader = (bool)_mpDevice->_mpAdapter->requestGPUSettings().tessellationSupported,
                                                        .supportRayTracing = (bool)_mpDevice->_mRaytracingSupported});

    auto dstStageMask = determine_pipeline_stage_flags({.mAccessFlags      = dstAccessFlags,
                                                        .queueType         = (QueueTypeFlag)_mType,
                                                        .supportGeomShader = (bool)_mpDevice->_mpAdapter->requestGPUSettings().geometryShaderSupported,
                                                        .supportTeseShader = (bool)_mpDevice->_mpAdapter->requestGPUSettings().tessellationSupported,
                                                        .supportRayTracing = (bool)_mpDevice->_mRaytracingSupported});

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