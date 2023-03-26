#pragma once
#include "02Rhi/Rhi.hpp"
#include "02Rhi/Vulkan/VulkanEnums.hpp"

class VmaAllocation_T;
using VmaAllocation = VmaAllocation_T*;

namespace axe::rhi
{
class VulkanDevice;
class VulkanCmd;
class VulkanBuffer : public Buffer
{
    friend class VulkanDevice;
    friend class VulkanCmd;
    AXE_NON_COPYABLE(VulkanBuffer);
    VulkanBuffer(VulkanDevice* device) noexcept : _mpDevice(device) {}
    bool _create(const BufferDesc&) noexcept;
    bool _destroy() noexcept;

public:
    ~VulkanBuffer() noexcept override = default;

public:
    auto handle() noexcept { return _mpHandle; }

public:
    constexpr static VkObjectType getVkTypeId() noexcept { return VK_OBJECT_TYPE_BUFFER; }

private:
    VulkanDevice* const _mpDevice      = nullptr;
    VkBuffer _mpHandle                 = VK_NULL_HANDLE;
    VkBufferView _mpVkStorageTexelView = VK_NULL_HANDLE;
    VkBufferView _mpVkUniformTexelView = VK_NULL_HANDLE;
    VmaAllocation _mpVkAllocation;  // Contains resource allocation info such as parent heap, offset in heap
    uint64_t _mOffset;

    void* _mpCPUMappedAddress = nullptr;

    u32 _mSize                = 0;
    u32 _mDescriptors : 20    = 0;
    u32 _mMemoryUsage : 3     = 0;
};

}  // namespace axe::rhi