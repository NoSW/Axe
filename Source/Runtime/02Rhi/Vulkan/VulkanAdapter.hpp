#pragma once

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
    ~VulkanAdapter() noexcept
    {
        release();
        //
    };
    void release() noexcept;
    u8 queueFamilyIndex(QueueType type) const noexcept { return type < _mQueueFamilyIndex.size() ? _mQueueFamilyIndex[type] : U8_MAX; }
    u8 nodeIndex() const noexcept { return _mNodeIndex; }
    bool idle() const noexcept { return (bool)_mIdle; }
    bool take() noexcept { return _mIdle ? !(_mIdle = 0) : false; }
    auto handle() const noexcept { return _mpHandle; }
    auto* features() const noexcept { return &_mpFeatures; }
    AdapterType type() const noexcept { return (AdapterType)_mpProperties.properties.deviceType; }
    auto backendHandle() const noexcept { return _mpBackend->handle(); }

    static bool isBetterGpu(const VulkanAdapter& a, const VulkanAdapter& b) noexcept;

public:
    AXE_PUBLIC Device* requestDevice(DeviceDesc&) noexcept override;
    AXE_PUBLIC void releaseDevice(Device*&) noexcept override;
    AXE_PUBLIC GPUSettings requestGPUSettings() noexcept override { return _mpGPUSettings; };

private:
    VulkanBackend* _mpBackend  = nullptr;
    VkPhysicalDevice _mpHandle = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties2 _mpProperties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
    VkPhysicalDeviceMemoryProperties2 _mpMemoryProperties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2};
    VkPhysicalDeviceFeatures2 _mpFeatures{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    GPUSettings _mpGPUSettings;

    std::array<std::unique_ptr<VulkanDevice>, MAX_NUM_DEVICE_PER_ADAPTER> _mDevices;

    u8 _mNodeIndex = U8_MAX;
    u8 _mIdle : 1  = 1;
    std::array<u8, QueueType::MAX_QUEUE_TYPE> _mQueueFamilyIndex{U8_MAX, U8_MAX, U8_MAX};
};

}  // namespace axe::rhi