#include "VulkanDescriptorSet.hxx"
#include "VulkanDevice.hxx"
#include "VulkanRootSignature.hxx"
#include "VulkanTexture.hxx"

#include <volk.h>

namespace axe::rhi
{
bool VulkanDescriptorSet::_create(DescriptorSetDesc& desc) noexcept
{
    // check if requested updateFreq is registered in RootSignature
    u8 updateFreq = (u8)desc.updateFrequency;
    AXE_ASSERT(updateFreq < (u8)DESCRIPTOR_UPDATE_FREQ_COUNT);
    VulkanRootSignature* pRootSignature = (VulkanRootSignature*)desc.mpRootSignature;

    if (pRootSignature[updateFreq]._mpDescriptorSetLayouts[updateFreq] == VK_NULL_HANDLE)
    {
        AXE_ERROR("DescriptorSetLayout in RootSignature is null at updateFreq={}", updateFreq);
        return false;
    }

    // populate this object
    _mpRootSignature = pRootSignature;
    _mpHandles.resize(desc.mMaxSet);
    _mDynamicUniformData.resize(pRootSignature->_mDynamicDescriptorCounts[updateFreq]);
    _mUpdateFrequency = updateFreq;

    // create descriptor pool and allocate descriptor sets from it
    {
        // create descriptor pool
        u32 poolSizeCount = pRootSignature->_mPoolSizeCount[updateFreq];
        std::array<VkDescriptorPoolSize, MAX_DESCRIPTOR_POOL_SIZE_ARRAY_COUNT> poolSizes;
        AXE_ASSERT(poolSizeCount <= MAX_DESCRIPTOR_POOL_SIZE_ARRAY_COUNT);
        for (u32 i = 0; i < poolSizeCount; ++i)
        {
            poolSizes[i].type            = pRootSignature->_mPoolSizes[updateFreq][i].type;
            poolSizes[i].descriptorCount = pRootSignature->_mPoolSizes[updateFreq][i].descriptorCount * desc.mMaxSet;
        }

        VkDescriptorPoolCreateInfo poolInfo{
            .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .flags         = 0,
            .maxSets       = desc.mMaxSet,
            .poolSizeCount = poolSizeCount,
            .pPoolSizes    = poolSizes.data(),
        };

        if (VK_FAILED(vkCreateDescriptorPool(_mpDevice->handle(), &poolInfo, nullptr, &_mpDescriptorPool))) { return false; }

        // allocate descriptor sets
        std::pmr::vector<VkDescriptorSetLayout> pDescriptorSetLayouts(desc.mMaxSet, _mpRootSignature->_mpDescriptorSetLayouts[updateFreq]);
        VkDescriptorSetAllocateInfo allocInfo{
            .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext              = nullptr,
            .descriptorPool     = _mpDescriptorPool,
            .descriptorSetCount = (u32)pDescriptorSetLayouts.size(),
            .pSetLayouts        = pDescriptorSetLayouts.data(),
        };

        if (VK_FAILED(vkAllocateDescriptorSets(_mpDevice->handle(), &allocInfo, _mpHandles.data()))) { return false; }
    }

    // do first update for all resources here to avoid missing-update of undefined resoureces
    {
        std::pmr::vector<VkDescriptorImageInfo> samplerInfos;
        std::pmr::vector<VkDescriptorBufferInfo> bufferInfos;
        std::pmr::vector<VkBufferView> bufferViewInfos;
        std::pmr::vector<VkBufferView> bufferViewRawInfos;

        auto getAppendCount = [](u32 currentCount, u32 leastRequiredCount) -> u32
        {
            return leastRequiredCount > currentCount ? (leastRequiredCount - currentCount) : 0;
        };

        for (const DescriptorInfo& descriptorInfo : pRootSignature->_mDescriptors)
        {
            if (descriptorInfo.updateFrequency != updateFreq || descriptorInfo.isRootDescriptor || descriptorInfo.isStaticSampler) { continue; }
            std::pmr::vector<VkDescriptorImageInfo> imageInfos;
            VkDescriptorImageInfo* pImageInfo   = nullptr;
            VkDescriptorBufferInfo* pBufferInfo = nullptr;
            VkBufferView* pTexelBufferView      = nullptr;

            auto updateHelper                   = [this, &descriptorInfo](VkDescriptorImageInfo* pImageInfo, VkDescriptorBufferInfo* pBufferInfo, VkBufferView* pTexelBufferView)
            {
                for (VkDescriptorSet handle : _mpHandles)
                {
                    VkWriteDescriptorSet writeSet{
                        .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                        .pNext            = nullptr,
                        .dstSet           = handle,
                        .dstBinding       = descriptorInfo.reg,
                        .dstArrayElement  = 0,
                        .descriptorCount  = descriptorInfo.size,
                        .descriptorType   = (VkDescriptorType)descriptorInfo.vkType,
                        .pImageInfo       = pImageInfo,
                        .pBufferInfo      = pBufferInfo,
                        .pTexelBufferView = pTexelBufferView,
                    };
                    vkUpdateDescriptorSets(_mpDevice->handle(), 1, &writeSet, 0, nullptr);
                }
            };

            switch (descriptorInfo.type)
            {
                case DESCRIPTOR_TYPE_SAMPLER:
                {
                    u32 appendCount = getAppendCount(samplerInfos.size(), descriptorInfo.size);
                    for (u32 i = 0; i < appendCount; ++i)
                    {
                        samplerInfos.push_back(VkDescriptorImageInfo{.sampler = _mpDevice->getDefaultSamplerHandle(), .imageView = VK_NULL_HANDLE, .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED});
                    }
                    updateHelper(samplerInfos.data(), nullptr, nullptr);
                }
                break;
                case DESCRIPTOR_TYPE_TEXTURE:
                {
                    std::pmr::vector<VkDescriptorImageInfo> imageInfos(
                        descriptorInfo.size,
                        VkDescriptorImageInfo{
                            .sampler     = VK_NULL_HANDLE,
                            .imageView   = ((VulkanTexture*)_mpDevice->_mNullDescriptors.pDefaultTextureSRV[descriptorInfo.dim])->_mpVkSRVDescriptor,
                            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
                    updateHelper(imageInfos.data(), nullptr, nullptr);
                };
                break;
                case DESCRIPTOR_TYPE_RW_TEXTURE:
                {
                    std::pmr::vector<VkDescriptorImageInfo> imageInfos(
                        descriptorInfo.size,
                        VkDescriptorImageInfo{
                            .sampler     = VK_NULL_HANDLE,
                            .imageView   = ((VulkanTexture*)_mpDevice->_mNullDescriptors.pDefaultTextureUAV[descriptorInfo.dim])->_mpVkUAVDescriptors[0],
                            .imageLayout = VK_IMAGE_LAYOUT_GENERAL});
                    updateHelper(imageInfos.data(), nullptr, nullptr);
                }
                break;
                case DESCRIPTOR_TYPE_BUFFER:
                case DESCRIPTOR_TYPE_BUFFER_RAW:
                case DESCRIPTOR_TYPE_RW_BUFFER:
                case DESCRIPTOR_TYPE_RW_BUFFER_RAW:
                case DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                {
                    u32 appendCount = getAppendCount(bufferInfos.size(), descriptorInfo.size);
                    for (u32 i = 0; i < appendCount; ++i)
                    {
                        bufferInfos.push_back(VkDescriptorBufferInfo{.buffer = _mpDevice->getDefaultBufferSRVHandle(), .offset = 0, .range = VK_WHOLE_SIZE});
                    }
                    updateHelper(nullptr, bufferInfos.data(), nullptr);
                }
                break;
                case DESCRIPTOR_TYPE_TEXEL_BUFFER_VKONLY:
                {
                    u32 appendCount = getAppendCount(bufferViewInfos.size(), descriptorInfo.size);
                    for (u32 i = 0; i < appendCount; ++i)
                    {
                        bufferViewInfos.push_back(((VulkanBuffer*)_mpDevice->_mNullDescriptors.pDefaultBufferSRV)->_mpVkUniformTexelView);
                    }
                    updateHelper(nullptr, nullptr, bufferViewInfos.data());
                }
                break;
                case DESCRIPTOR_TYPE_RW_TEXEL_BUFFER_VKONLY:
                {
                    u32 appendCount = getAppendCount(bufferViewRawInfos.size(), descriptorInfo.size);
                    for (u32 i = 0; i < appendCount; ++i)
                    {
                        bufferViewRawInfos.push_back(((VulkanBuffer*)_mpDevice->_mNullDescriptors.pDefaultBufferUAV)->_mpVkStorageTexelView);
                    }
                    updateHelper(nullptr, nullptr, bufferViewRawInfos.data());
                }
                break;
                default: AXE_ERROR("Unsupported"); break;
            }
        }
    }
    return true;
}

bool VulkanDescriptorSet::_destroy() noexcept
{
    AXE_ASSERT(_mpDevice);
    vkDestroyDescriptorPool(_mpDevice->handle(), _mpDescriptorPool, &g_VkAllocationCallbacks);
    return true;
}

void VulkanDescriptorSet::update(u32 index, std::pmr::vector<DescriptorData*> params) noexcept
{
    for (const DescriptorData* pParam : params)
    {
        // Step 1: find descriptor info
        u32 paramIndex                             = pParam->index;
        const DescriptorInfo* pFoundDescriptorInfo = nullptr;
        if (paramIndex < _mpRootSignature->_mDescriptors.size())
        {
            pFoundDescriptorInfo = &_mpRootSignature->_mDescriptors[paramIndex];
        }
        else if (!pParam->name.empty())
        {
            auto iter = _mpRootSignature->_mNameHashIds.find(std::pmr::string(pParam->name));
            if (iter != _mpRootSignature->_mNameHashIds.end())
            {
                pFoundDescriptorInfo = &_mpRootSignature->_mDescriptors[iter->second];
            }
        }

        if (pFoundDescriptorInfo == nullptr)
        {
            AXE_ERROR("Failed to find descriptor param with index={}, or name {}", paramIndex, pParam->name);
            continue;
        }

        // Step 2: do some checks
        std::string_view foundName = pFoundDescriptorInfo->name;
        if (pFoundDescriptorInfo->updateFrequency != _mUpdateFrequency)
        {
            AXE_ERROR("Failed to update descriptor since update frequency mismatch, name={}", foundName);
            continue;
        }
        if (pParam->pResources.empty())
        {
            AXE_ERROR("Failed to update descriptor since count is 0, name={}", foundName);
            continue;
        }

        // Step 3: update descriptor set by type
        auto updateHelper = [this, pFoundDescriptorInfo, pParam, index](
                                VkDescriptorImageInfo* pImageInfo, VkDescriptorBufferInfo* pBufferInfo, VkBufferView* pTexelBufferView, u32 descriptorCount)
        {
            VkWriteDescriptorSet writeSet{
                .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext            = nullptr,
                .dstSet           = _mpHandles[index],
                .dstBinding       = pFoundDescriptorInfo->reg,
                .dstArrayElement  = pParam->arrayOffset,
                .descriptorCount  = descriptorCount,
                .descriptorType   = (VkDescriptorType)pFoundDescriptorInfo->vkType,
                .pImageInfo       = pImageInfo,
                .pBufferInfo      = pBufferInfo,
                .pTexelBufferView = pTexelBufferView,
            };
            vkUpdateDescriptorSets(_mpDevice->handle(), 1, &writeSet, 0, nullptr);
        };

        switch (pFoundDescriptorInfo->type)
        {
            case DESCRIPTOR_TYPE_SAMPLER:
            {
                if (pFoundDescriptorInfo->isStaticSampler)
                {
                    AXE_ERROR("Trying to update static sampler that is NOT allowed, name={}", foundName);
                    continue;
                }
                std::pmr::vector<VkDescriptorImageInfo> imageInfos;
                imageInfos.reserve(pParam->pResources.size());
                for (void* pResource : pParam->pResources)
                {
                    imageInfos.push_back(VkDescriptorImageInfo{
                        .sampler = ((VulkanSampler*)pResource)->handle(), .imageView = VK_NULL_HANDLE, .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED});
                }
                updateHelper(imageInfos.data(), nullptr, nullptr, pParam->pResources.size());
                break;
            }
            case DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER_VKONLY:
            {
                std::pmr::vector<VkDescriptorImageInfo> imageInfos;
                imageInfos.reserve(pParam->pResources.size());
                for (void* pResource : pParam->pResources)
                {
                    imageInfos.push_back(VkDescriptorImageInfo{
                        .sampler     = VK_NULL_HANDLE,
                        .imageView   = ((VulkanTexture*)pResource)->_mpVkSRVDescriptor,
                        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
                }
                updateHelper(imageInfos.data(), nullptr, nullptr, pParam->pResources.size());
                break;
            }
            case DESCRIPTOR_TYPE_TEXTURE:
            {
                std::pmr::vector<VkDescriptorImageInfo> imageInfos;
                imageInfos.reserve(pParam->pResources.size());
                for (void* pResource : pParam->pResources)
                {
                    imageInfos.push_back(VkDescriptorImageInfo{
                        .sampler     = VK_NULL_HANDLE,
                        .imageView   = pParam->isBindStencilResource ? ((VulkanTexture*)pResource)->_mpVkSRVStencilDescriptor : ((VulkanTexture*)pResource)->_mpVkSRVDescriptor,
                        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
                }
                updateHelper(imageInfos.data(), nullptr, nullptr, pParam->pResources.size());
                break;
            }
            case DESCRIPTOR_TYPE_RW_TEXTURE:
            {
                std::pmr::vector<VkDescriptorImageInfo> imageInfos;

                if (pParam->isBindMipChain)
                {
                    if (pParam->arrayOffset != 0)
                    {
                        AXE_ERROR("RWTexture array offset must be 0 when binding mip chain, name={}", foundName);
                        continue;
                    }
                    const u32 mipCount = ((VulkanTexture*)(pParam->pResources[0]))->_mMipLevels;
                    imageInfos.reserve(mipCount);
                    for (u32 i = 0; i < mipCount; ++i)
                    {
                        imageInfos.push_back(VkDescriptorImageInfo{
                            .sampler     = VK_NULL_HANDLE,
                            .imageView   = ((VulkanTexture*)(pParam->pResources[0]))->_mpVkUAVDescriptors[i],
                            .imageLayout = VK_IMAGE_LAYOUT_GENERAL});
                    }
                    updateHelper(imageInfos.data(), nullptr, nullptr, mipCount);
                }
                else
                {
                    const u32 mipSlice = pParam->UAVMipSlice;
                    imageInfos.reserve(pParam->pResources.size());
                    for (void* pResource : pParam->pResources)
                    {
                        imageInfos.push_back(VkDescriptorImageInfo{
                            .sampler     = VK_NULL_HANDLE,
                            .imageView   = ((VulkanTexture*)pResource)->_mpVkUAVDescriptors[mipSlice],
                            .imageLayout = VK_IMAGE_LAYOUT_GENERAL});
                    }
                    updateHelper(imageInfos.data(), nullptr, nullptr, pParam->pResources.size());
                }
                break;
            }
            case DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            {
                if (pFoundDescriptorInfo->isRootDescriptor)
                {
                    AXE_ERROR("Descriptor (name={}) tries to update a root cbv through updateDescriptorSet. All root cbvs must be updated through bindDescriptorSetWithRootCbvs", pFoundDescriptorInfo->name);
                    break;
                }
            }
            case DESCRIPTOR_TYPE_BUFFER:
            case DESCRIPTOR_TYPE_BUFFER_RAW:
            case DESCRIPTOR_TYPE_RW_BUFFER:
            case DESCRIPTOR_TYPE_RW_BUFFER_RAW:
            {
                std::pmr::vector<VkDescriptorBufferInfo> bufferInfos;
                bufferInfos.reserve(pParam->pResources.size());
                for (u32 i = 0; i < pParam->pResources.size(); ++i)
                {
                    void* pResource     = pParam->pResources[i];
                    VkDeviceSize offset = pParam->pRanges ? pParam->pRanges[i].offset : ((VulkanBuffer*)pResource)->_mOffset;
                    VkDeviceSize range  = pParam->pRanges ? pParam->pRanges[i].size : VK_WHOLE_SIZE;

                    u32 maxRange        = DESCRIPTOR_TYPE_UNIFORM_BUFFER == pFoundDescriptorInfo->type ?
                                              _mpDevice->_mpAdapter->maxUniformBufferRange() :
                                              _mpDevice->_mpAdapter->maxStorageBufferRange();
                    if (range == 0 || range > maxRange)
                    {
                        AXE_ERROR(" Descriptor(name={})'s ranges[{}].size is {} which is not in [1, {}]", pFoundDescriptorInfo->name, i, (u32)range, maxRange);
                    }

                    bufferInfos.push_back(VkDescriptorBufferInfo{
                        .buffer = ((VulkanBuffer*)pResource)->handle(),
                        .offset = offset,
                        .range  = range});
                }
                updateHelper(nullptr, bufferInfos.data(), nullptr, pParam->pResources.size());
                break;
            }
            case DESCRIPTOR_TYPE_TEXEL_BUFFER_VKONLY:
            {
                std::pmr::vector<VkBufferView> bufferViews;
                bufferViews.reserve(pParam->pResources.size());
                for (void* pResource : pParam->pResources) { bufferViews.push_back(((VulkanBuffer*)pResource)->_mpVkUniformTexelView); }
                updateHelper(nullptr, nullptr, bufferViews.data(), pParam->pResources.size());
                break;
            }
            case DESCRIPTOR_TYPE_RW_TEXEL_BUFFER_VKONLY:
            {
                std::pmr::vector<VkBufferView> bufferViews;
                bufferViews.reserve(pParam->pResources.size());
                for (void* pResource : pParam->pResources) { bufferViews.push_back(((VulkanBuffer*)pResource)->_mpVkStorageTexelView); }
                updateHelper(nullptr, nullptr, bufferViews.data(), pParam->pResources.size());
                break;
            }
            case DESCRIPTOR_TYPE_RAY_TRACING:
            {
                AXE_ERROR("Unsupported");
                break;
            }
            default: AXE_ERROR("Unsupported"); break;
        }
    }
}

}  // namespace axe::rhi