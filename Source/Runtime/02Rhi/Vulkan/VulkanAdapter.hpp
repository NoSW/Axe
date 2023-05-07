#pragma once
#include "02Rhi/Rhi.hpp"
#include "02Rhi/Vulkan/VulkanEnums.hpp"
#include "02Rhi/Vulkan/VulkanBackend.hpp"

namespace axe::rhi
{
class VulkanDevice;
class VulkanQueue;
class VulkanSwapChain;

class VulkanAdapter final : public Adapter
{
public:
    VulkanAdapter(VulkanBackend* backend, VkPhysicalDevice handle, u8 nodeIndex) noexcept;
    ~VulkanAdapter() noexcept { busy2idle(); }
    u8 nodeIndex() const noexcept { return _mNodeIndex; }
    bool idle2busy() noexcept { return _mIdle ? !(_mIdle = 0) : false; }
    void busy2idle() noexcept
    {
        for (auto& d : _mDevices) { AXE_ASSERT(d.get() == nullptr); }
        _mIdle = true;
    }
    auto handle() const noexcept { return _mpHandle; }
    auto* features() const noexcept { return &_mpFeatures; }
    AdapterType type() const noexcept { return (AdapterType)_mpProperties.properties.deviceType; }
    auto backendHandle() const noexcept { return _mpBackend->handle(); }

    bool isSupportRead(TinyImageFormat format) const noexcept { return _mGPUCapBits.mCanShaderReadFrom[format]; }
    bool isSupportWrite(TinyImageFormat format) const noexcept { return _mGPUCapBits.mCanShaderWriteTo[format]; }
    bool isSupportRenderTargetWrite(TinyImageFormat format) const noexcept { return _mGPUCapBits.mCanRenderTargetWriteTo[format]; }
    bool isSupportQueue(QueueType) noexcept;
    u32 maxUniformBufferRange() const noexcept { return _mpProperties.properties.limits.maxUniformBufferRange; }
    u32 maxStorageBufferRange() const noexcept { return _mpProperties.properties.limits.maxStorageBufferRange; }

    static bool isBetterGpu(const VulkanAdapter& a, const VulkanAdapter& b) noexcept;

    static bool isDedicatedQueue(VkQueueFlags quFlags) noexcept;

public:
    AXE_PUBLIC Device* requestDevice(DeviceDesc&) noexcept override;
    AXE_PUBLIC void releaseDevice(Device*&) noexcept override;
    AXE_PUBLIC const GPUSettings& requestGPUSettings() const noexcept override { return _mpGPUSettings; };

public:
    constexpr static VkObjectType getVkTypeId() noexcept { return VK_OBJECT_TYPE_PHYSICAL_DEVICE; }

private:
    // ref
    VulkanBackend* _mpBackend  = nullptr;

    // resource-handle
    VkPhysicalDevice _mpHandle = VK_NULL_HANDLE;

    // scalar
    VkPhysicalDeviceProperties2 _mpProperties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
    VkPhysicalDeviceMemoryProperties2 _mpMemoryProperties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2};
    VkPhysicalDeviceFeatures2 _mpFeatures{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    GPUSettings _mpGPUSettings;
    GPUCapBits _mGPUCapBits;

    u8 _mNodeIndex                     = U8_MAX;
    u8 _mIdle                      : 1 = 1;
    u8 _mSupportGraphicsQueue      : 1 = 0;
    u8 _mSupportComputeQueue       : 1 = 0;
    u8 _mSupportTransferQueue      : 1 = 0;
    u8 _mHasDedicatedComputeQueue  : 1 = 0;
    u8 _mHasDedicatedTransferQueue : 1 = 0;

    // auto
    std::array<std::unique_ptr<VulkanDevice>, MAX_NUM_DEVICE_PER_ADAPTER> _mDevices;
};

}  // namespace axe::rhi