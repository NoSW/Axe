#pragma once
#include "02Rhi/Rhi.hpp"
#include "VulkanEnums.internal.hpp"

namespace axe::rhi
{

class VulkanRootSignature;
class VulkanDevice;
class VulkanCmd;

struct DynamicUniformData
{
    VkBuffer pBuffer = VK_NULL_HANDLE;
    u32 offset       = 0;
    u32 size         = 0;
};

class VulkanDescriptorSet final : public DescriptorSet
{
    friend class VulkanDevice;
    friend class VulkanCmd;

public:
    VulkanDescriptorSet(VulkanDevice* device) noexcept : _mpDevice(device) {}
    bool _create(DescriptorSetDesc&) noexcept;
    bool _destroy() noexcept;

    ~VulkanDescriptorSet() override = default;
    auto handle() noexcept { return _mpDescriptorPool; }

public:
    void update(u32 index, std::pmr::vector<DescriptorData*> params) noexcept override;

public:
    constexpr static auto VK_TYPE_ID = VK_OBJECT_TYPE_DESCRIPTOR_POOL;

private:
    std::pmr::vector<VkDescriptorSet> _mpHandles;
    const VulkanRootSignature* _mpRootSignature = nullptr;
    VulkanDevice* const _mpDevice               = nullptr;
    std::pmr::vector<DynamicUniformData> _mDynamicUniformData;
    VkDescriptorPool _mpDescriptorPool = VK_NULL_HANDLE;
    u8 _mUpdateFrequency               = 0;  // current set, e.g., UPDATE_FREQ_NONE => layout(set = 0)
};

}  // namespace axe::rhi