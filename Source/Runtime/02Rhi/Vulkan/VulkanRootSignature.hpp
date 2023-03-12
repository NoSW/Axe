#pragma once

#include "02Rhi/Rhi.hpp"
#include "02Rhi/Vulkan/VulkanEnums.hpp"

namespace axe::rhi
{
class VulkanDevice;
class VulkanRootSignature : public RootSignature
{
private:
    AXE_NON_COPYABLE(VulkanRootSignature);
    VulkanRootSignature() noexcept = default;
    bool _create(RootSignatureDesc&) noexcept;
    bool _destroy() noexcept;
    friend class VulkanDevice;

private:
    // created from
    VulkanDevice* const _mpDevice = nullptr;

    // common data
    std::pmr::vector<DescriptorInfo> _mDescriptors;
    std::pmr::vector<u32> _mNameHashIds;
    PipelineType _mPipelineType        = PIPELINE_TYPE_UNDEFINED;

    // vulkan specific data
    VkPipelineLayout _mpPipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout _mpDescriptorSetLayouts[DESCRIPTOR_UPDATE_FREQ_COUNT]{};
    u8 _mDynamicDescriptorCounts[DESCRIPTOR_UPDATE_FREQ_COUNT]{};
    VkDescriptorPoolSize _mPoolSizes[DESCRIPTOR_UPDATE_FREQ_COUNT][MAX_DESCRIPTOR_POOL_SIZE_ARRAY_COUNT];
    u8 _mPoolSizeCount[DESCRIPTOR_UPDATE_FREQ_COUNT]{};

    VkDescriptorPool _mpEmptyDescriptorPool[DESCRIPTOR_UPDATE_FREQ_COUNT]{};
    VkDescriptorSet _mpEmptyDescriptorSet[DESCRIPTOR_UPDATE_FREQ_COUNT]{};
};

}  // namespace axe::rhi