#pragma once
#include "02Rhi/Rhi.hpp"
#include "VulkanEnums.hxx"

class VmaAllocation_T;
using VmaAllocation = VmaAllocation_T*;

namespace axe::rhi
{
class VulkanDevice;
class VulkanCmd;
class VulkanDescriptorSet;
class VulkanBuffer : public Buffer
{
    friend class VulkanDevice;
    friend class VulkanCmd;
    friend class VulkanDescriptorSet;
    AXE_NON_COPYABLE(VulkanBuffer);
    VulkanBuffer(VulkanDevice* device) noexcept : _mpDevice(device) {}
    bool _create(const BufferDesc&) noexcept;
    bool _destroy() noexcept;

public:
    ~VulkanBuffer() noexcept override = default;

public:
    AXE_PRIVATE auto handle() noexcept { return _mpHandle; }

public:
    [[nodiscard]] void* addressOfCPU() noexcept override { return _mpCPUMappedAddress; }
    [[nodiscard]] u64 offset() const noexcept override { return _mOffset; }
    [[nodiscard]] u64 size() const noexcept override { return _mSize; }
    [[nodiscard]] ResourceMemoryUsage memoryUsage() const noexcept override { return _mMemoryUsage; }
    void* map() noexcept override;
    void unmap() noexcept override;

public:
    constexpr static auto VK_TYPE_ID = VK_OBJECT_TYPE_BUFFER;

private:
    VulkanDevice* const _mpDevice             = nullptr;
    VkBuffer _mpHandle                        = VK_NULL_HANDLE;
    VkBufferView _mpVkStorageTexelView        = VK_NULL_HANDLE;
    VkBufferView _mpVkUniformTexelView        = VK_NULL_HANDLE;
    VmaAllocation _mpVkAllocation             = nullptr;  // Contains resource allocation info such as parent heap, offset in heap

    void* _mpCPUMappedAddress                 = nullptr;
    u32 _mSize                                = 0;
    u64 _mOffset                              = 0;

    DescriptorTypeFlagOneBit _mDescriptorType = DescriptorTypeFlag::UNDEFINED;
    ResourceMemoryUsage _mMemoryUsage         = ResourceMemoryUsage::UNKNOWN;
};

}  // namespace axe::rhi