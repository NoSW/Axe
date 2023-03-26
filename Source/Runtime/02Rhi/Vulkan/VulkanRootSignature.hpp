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

public:
    AXE_PUBLIC ~VulkanRootSignature() noexcept override { AXE_ASSERT(_mpPipelineLayout == VK_NULL_HANDLE); }

public:
    auto handle() noexcept { return _mpPipelineLayout; }

public:
    constexpr static VkObjectType getVkTypeId() noexcept { return VK_OBJECT_TYPE_PIPELINE_LAYOUT; }

private:
    // created from
    VulkanDevice* const _mpDevice = nullptr;

    // common data
    std::pmr::vector<DescriptorInfo> _mDescriptors;
    std::pmr::vector<u32> _mNameHashIds;
    PipelineType _mPipelineType        = PIPELINE_TYPE_UNDEFINED;

    // vulkan specific data (how to manage descriptor: https://www.khronos.org/blog/vk-ext-descriptor-buffer)
    // how to specify how many of each descriptor we need to allocate in each pool:
    //  1. Pretending it's a linear pool, just allocate a bunch until hit VK_ERROR_OUT_OF_POOL_MEMORY,
    //     make a new one and throw old away, which mirrors how we would use command allocators.
    //  2. Slab allocator per unique VkDescriptorSetLayout.
    //        pros: we can recycle VkDescriptorSet objects themselves instead of allocating them all the time
    //        cons: we need more pools
    //  3. One pool per rendering thread.
    //        pros:
    //        cons: gets wasteful very quickly, especially whin linear allocator method.
    //              (using a thread-safe ring buffer (allocating N descriptor sets in one go) may alleviate it)

    VkPipelineLayout _mpPipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout _mpDescriptorSetLayouts[DESCRIPTOR_UPDATE_FREQ_COUNT]{};
    u8 _mDynamicDescriptorCounts[DESCRIPTOR_UPDATE_FREQ_COUNT]{};
    VkDescriptorPoolSize _mPoolSizes[DESCRIPTOR_UPDATE_FREQ_COUNT][MAX_DESCRIPTOR_POOL_SIZE_ARRAY_COUNT];
    u8 _mPoolSizeCount[DESCRIPTOR_UPDATE_FREQ_COUNT]{};

    VkDescriptorPool _mpEmptyDescriptorPool[DESCRIPTOR_UPDATE_FREQ_COUNT]{};
    VkDescriptorSet _mpEmptyDescriptorSet[DESCRIPTOR_UPDATE_FREQ_COUNT]{};
};

}  // namespace axe::rhi