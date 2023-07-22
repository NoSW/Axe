#pragma once
#include "02Rhi/Rhi.hpp"
#include "VulkanEnums.internal.hpp"
#include "VulkanBackend.internal.hpp"

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
    AXE_PRIVATE u8 nodeIndex() const noexcept { return _mNodeIndex; }
    AXE_PRIVATE bool idle2busy() noexcept { return _mIdle ? !(_mIdle = 0) : false; }
    AXE_PRIVATE void busy2idle() noexcept
    {
        for (auto& d : _mDevices) { AXE_ASSERT(d.get() == nullptr); }
        _mIdle = true;
    }
    AXE_PRIVATE auto handle() const noexcept { return _mpHandle; }
    AXE_PRIVATE auto* features() const noexcept { return &_mpFeatures; }
    AXE_PRIVATE AdapterType type() const noexcept { return (AdapterType)_mpProperties.properties.deviceType; }
    AXE_PRIVATE auto backendHandle() const noexcept { return _mpBackend->handle(); }

    AXE_PRIVATE bool isSupportRead(TinyImageFormat format) const noexcept { return _mGPUCapBits.canShaderReadFrom[format]; }
    AXE_PRIVATE bool isSupportWrite(TinyImageFormat format) const noexcept { return _mGPUCapBits.canShaderWriteTo[format]; }
    AXE_PRIVATE bool isSupportRenderTargetWrite(TinyImageFormat format) const noexcept { return _mGPUCapBits.canRenderTargetWriteTo[format]; }
    AXE_PRIVATE bool isSupportQueue(QueueTypeFlag) noexcept;
    AXE_PRIVATE u32 maxUniformBufferRange() const noexcept { return _mpProperties.properties.limits.maxUniformBufferRange; }
    AXE_PRIVATE u32 maxStorageBufferRange() const noexcept { return _mpProperties.properties.limits.maxStorageBufferRange; }

    AXE_PRIVATE static bool isBetterGpu(const VulkanAdapter& a, const VulkanAdapter& b) noexcept;

    AXE_PRIVATE static bool isDedicatedQueue(VkQueueFlags quFlags) noexcept;

public:
    Device* requestDevice(DeviceDesc&) noexcept override;
    void releaseDevice(Device*&) noexcept override;
    const GPUSettings& requestGPUSettings() const noexcept override { return _mpGPUSettings; };

public:
    constexpr static auto VK_TYPE_ID = VK_OBJECT_TYPE_PHYSICAL_DEVICE;

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