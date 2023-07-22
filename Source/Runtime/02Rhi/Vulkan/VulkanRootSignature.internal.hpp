#pragma once

#include "02Rhi/Rhi.hpp"
#include "VulkanEnums.internal.hpp"

namespace axe::rhi
{
class VulkanDevice;
class VulkanCmd;
class VulkanDescriptorSet;
class VulkanRootSignature : public RootSignature
{
private:
    AXE_NON_COPYABLE(VulkanRootSignature);
    VulkanRootSignature(VulkanDevice* device) noexcept : _mpDevice(device) {}
    bool _create(RootSignatureDesc&) noexcept;
    bool _destroy() noexcept;
    friend class VulkanDevice;
    friend class VulkanCmd;
    friend class VulkanDescriptorSet;

public:
    ~VulkanRootSignature() noexcept override { AXE_ASSERT(_mpPipelineLayout == VK_NULL_HANDLE); }

public:
    AXE_PRIVATE auto handle() noexcept { return _mpPipelineLayout; }

public:
    AXE_PRIVATE constexpr static auto VK_TYPE_ID = VK_OBJECT_TYPE_PIPELINE_LAYOUT;

private:
    // created from
    VulkanDevice* const _mpDevice = nullptr;

    // common data
    std::pmr::vector<DescriptorInfo> _mDescriptors;
    std::pmr::unordered_map<std::pmr::string, u32> _mNameHashIds;
    PipelineType _mPipelineType        = PipelineType::UNDEFINED;

    // descriptor set management:
    //   - experienced based: free lists of fixed descriptor set pools
    //   - slot based, sorted by descriptor type
    //   - updated freq based , sorted by update freq
    //   - bindless totally, bind at once

    // vulkan specific data (how to manage descriptor: https://www.khronos.org/blog/vk-ext-descriptor-buffer)
    // how to specify how many of each descriptor we need to allocate in each pool:
    //  1. Pretending it's a linear pool, just allocate a bunch until hit VK_ERROR_OUT_OF_POOL_MEMORY,
    //     make a new one and throw old away, which mirrors how we would use command allocators.
    //  2. Slab allocator per unique VkDescriptorSetLayout. (Adopted by Axe)
    //        pros: we can recycle VkDescriptorSet objects themselves instead of allocating them all the time
    //        cons: we need more pools
    //  3. One pool per rendering thread.
    //        pros:
    //        cons: gets wasteful very quickly, especially whin linear allocator method.
    //              (using a thread-safe ring buffer (allocating N descriptor sets in one go) may alleviate it)

    VkPipelineLayout _mpPipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout _mpDescriptorSetLayouts[(u32)DescriptorUpdateFrequency::COUNT]{};                         // layout for each set
    u8 _mDynamicDescriptorCounts[(u32)DescriptorUpdateFrequency::COUNT]{};                                          // current count of descriptor for each set
    VkDescriptorPoolSize _mPoolSizes[(u32)DescriptorUpdateFrequency::COUNT][MAX_DESCRIPTOR_POOL_SIZE_ARRAY_COUNT];  // <set id, pool id>
    u8 _mPoolSizeCount[(u32)DescriptorUpdateFrequency::COUNT]{};                                                    // current count of descriptor pool for each set
};

}  // namespace axe::rhi