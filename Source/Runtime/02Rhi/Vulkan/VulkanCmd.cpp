#include "VulkanCmd.hxx"
#include "VulkanCmdPool.hxx"
#include "VulkanQueue.hxx"
#include "VulkanTexture.hxx"
#include "VulkanBuffer.hxx"
#include "VulkanDescriptorSet.hxx"
#include "VulkanDevice.hxx"

#include <tiny_imageformat/tinyimageformat_query.h>
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
        for (u32 i = 0; i < (u32)DescriptorUpdateFrequency::COUNT; ++i)
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
        if (info.isAcquire && info.currentState != ResourceStateFlags::UNDEFINED)
        {
            return {_mpDevice->_mQueueFamilyIndexes[(u32)info.queueType], (u8)_mpQueue->_mVkQueueFamilyIndex};
        }
        else if (info.isRelease && info.currentState != ResourceStateFlags::UNDEFINED)
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

            const bool isBothUA                 = pTrans.barrierInfo.currentState == ResourceStateFlags::UNORDERED_ACCESS && pTrans.barrierInfo.newState == ResourceStateFlags::UNORDERED_ACCESS;
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
        const bool isBothUA                         = pTrans.barrierInfo.currentState == ResourceStateFlags::UNORDERED_ACCESS && pTrans.barrierInfo.newState == ResourceStateFlags::UNORDERED_ACCESS;
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

    VkPipelineStageFlags srcStageMask = determine_pipeline_stage_flags(
        DeterminePipelineStageOption{.mAccessFlags      = srcAccessFlags,
                                     .queueType         = (QueueTypeFlag)_mType,
                                     .supportGeomShader = (bool)_mpDevice->_mpAdapter->requestGPUSettings().geometryShaderSupported,
                                     .supportTeseShader = (bool)_mpDevice->_mpAdapter->requestGPUSettings().tessellationSupported,
                                     .supportRayTracing = (bool)_mpDevice->_mRaytracingSupported});

    VkPipelineStageFlags dstStageMask = determine_pipeline_stage_flags(
        DeterminePipelineStageOption{.mAccessFlags      = dstAccessFlags,
                                     .queueType         = (QueueTypeFlag)_mType,
                                     .supportGeomShader = (bool)_mpDevice->_mpAdapter->requestGPUSettings().geometryShaderSupported,
                                     .supportTeseShader = (bool)_mpDevice->_mpAdapter->requestGPUSettings().tessellationSupported,
                                     .supportRayTracing = (bool)_mpDevice->_mRaytracingSupported});

    vkCmdPipelineBarrier(_mpHandle, srcStageMask, dstStageMask, 0,
                         0, nullptr,
                         (u32)bufMemBarriers.size(), bufMemBarriers.data(),
                         (u32)imgMemBarriers.size(), imgMemBarriers.data());
}

void VulkanCmd::copyBuffer(Buffer* pDst, Buffer* pSrc, u64 srcOffset, u64 dstOffset, u64 size) noexcept
{
    AXE_ASSERT(pDst != nullptr);
    AXE_ASSERT(pSrc != nullptr);
    AXE_ASSERT(size > 0);
    AXE_ASSERT(dstOffset + size <= pDst->size());
    AXE_ASSERT(srcOffset + size <= pSrc->size());

    VkBufferCopy copy{
        .srcOffset = srcOffset,
        .dstOffset = dstOffset,
        .size      = size,
    };
    vkCmdCopyBuffer(_mpHandle, ((VulkanBuffer*)pSrc)->handle(), ((VulkanBuffer*)pDst)->handle(), 1, &copy);
}

static void update_subresource_impl(VulkanTexture* pVulkanTexture, VulkanBuffer* pVulkanBuffer, const SubresourceDataDesc& subresourceDesc,
                                    std::pmr::vector<VkBufferImageCopy>& regions) noexcept
{
    const TinyImageFormat fmt = (TinyImageFormat)pVulkanTexture->format();
    const bool isSinglePlane  = TinyImageFormat_IsSinglePlane(fmt);

    const u32 width           = pVulkanTexture->width();
    const u32 height          = pVulkanTexture->height();
    const u32 depth           = pVulkanTexture->depth();
    if (isSinglePlane)
    {
        const u32 w             = std::max<u32>(1, width >> subresourceDesc.mipLevel);
        const u32 h             = std::max<u32>(1, height >> subresourceDesc.mipLevel);
        const u32 d             = std::max<u32>(1, depth >> subresourceDesc.mipLevel);
        const u32 numBlocksWide = subresourceDesc.rowPitch / byte_count_of_format(fmt);
        const u32 numBlocksHigh = (subresourceDesc.slicePitch / subresourceDesc.rowPitch);

        regions.push_back(VkBufferImageCopy{
            .bufferOffset      = (VkDeviceSize)subresourceDesc.srcOffset,
            .bufferRowLength   = numBlocksWide * TinyImageFormat_WidthOfBlock(fmt),
            .bufferImageHeight = numBlocksHigh * TinyImageFormat_HeightOfBlock(fmt),
            .imageSubresource{
                .aspectMask     = (VkImageAspectFlags)pVulkanTexture->aspectMask(),
                .mipLevel       = subresourceDesc.mipLevel,
                .baseArrayLayer = subresourceDesc.arrayLayer,
                .layerCount     = 1,
            },
            .imageOffset{
                .x = 0,
                .y = 0,
                .z = 0,
            },
            .imageExtent{
                .width  = w,
                .height = h,
                .depth  = d,
            },
        });
    }
    else
    {
        const u32 numOfPlanes = TinyImageFormat_NumOfPlanes(fmt);

        uint64_t offset       = subresourceDesc.srcOffset;

        for (u32 i = 0; i < numOfPlanes; ++i)
        {
            regions.push_back(
                VkBufferImageCopy{
                    .bufferOffset      = (VkDeviceSize)offset,
                    .bufferRowLength   = 0u,
                    .bufferImageHeight = 0u,
                    .imageSubresource{
                        .aspectMask     = (u32)((VkImageAspectFlagBits)(VK_IMAGE_ASPECT_PLANE_0_BIT << i)),
                        .mipLevel       = subresourceDesc.mipLevel,
                        .baseArrayLayer = subresourceDesc.arrayLayer,
                        .layerCount     = 1,
                    },
                    .imageOffset{
                        .x = 0,
                        .y = 0,
                        .z = 0,
                    },
                    .imageExtent{
                        .width  = TinyImageFormat_PlaneWidth(fmt, i, width),
                        .height = TinyImageFormat_PlaneHeight(fmt, i, height),
                        .depth  = depth,
                    },
                });
            VkBufferImageCopy& copy = regions.back();
            offset += copy.imageExtent.width * copy.imageExtent.height * TinyImageFormat_PlaneSizeOfBlock(fmt, i);
        }
    }
}

void VulkanCmd::updateSubresource(Texture* pDstTexture, Buffer* pSrcBuffer, const SubresourceDataDesc& subresourceDesc) noexcept
{
    auto* pVulkanTexture = static_cast<VulkanTexture*>(pDstTexture);
    auto* pVulkanBuffer  = static_cast<VulkanBuffer*>(pSrcBuffer);
    std::pmr::vector<VkBufferImageCopy> regions;
    update_subresource_impl(pVulkanTexture, pVulkanBuffer, subresourceDesc, regions);
    vkCmdCopyBufferToImage(_mpHandle, pVulkanBuffer->handle(), pVulkanTexture->handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, (u32)regions.size(), regions.data());
}

void VulkanCmd::updateSubresource(Buffer* pDstBuffer, Texture* pSrcTexture, const SubresourceDataDesc& subresourceDesc) noexcept
{
    auto* pVulkanTexture = static_cast<VulkanTexture*>(pSrcTexture);
    auto* pVulkanBuffer  = static_cast<VulkanBuffer*>(pDstBuffer);
    std::pmr::vector<VkBufferImageCopy> regions;
    update_subresource_impl(pVulkanTexture, pVulkanBuffer, subresourceDesc, regions);
    vkCmdCopyImageToBuffer(_mpHandle, pVulkanTexture->handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, pVulkanBuffer->handle(), (u32)regions.size(), regions.data());
}

void VulkanCmd::resetQueryPool() noexcept {}

void VulkanCmd::beginQuery() noexcept {}

void VulkanCmd::endQuery() noexcept {}

void VulkanCmd::resolveQuery() noexcept {}

void VulkanCmd::addDebugMarker() noexcept {}

void VulkanCmd::beginDebugMarker() noexcept {}

void VulkanCmd::writeDebugMarker() noexcept {}

void VulkanCmd::endDebugMarker() noexcept {}

}  // namespace axe::rhi