#pragma once

#include "02Rhi/Rhi.hpp"
#include "02Rhi/Vulkan/VulkanEnums.hpp"

#include "00Core/Thread/Thread.hpp"

namespace axe::thread
{
class Mutex;
}

namespace axe::rhi
{
class VulkanDevice;
class VulkanQueue final : public Queue
{
    friend class VulkanDevice;
    friend class VulkanCmdPool;
    friend class VulkanCmd;
    friend class VulkanSwapChain;
    AXE_NON_COPYABLE(VulkanQueue);
    VulkanQueue(VulkanDevice* device) noexcept : _mpDevice(device) {}
    bool _create(QueueDesc&) noexcept;
    bool _destroy() noexcept;

public:
    ~VulkanQueue() noexcept override = default;
    void submit(QueueSubmitDesc& desc) noexcept override;
    void present(QueuePresentDesc& desc) noexcept override;
    void waitIdle() noexcept override;

private:
    VkQueue _mpHandle             = VK_NULL_HANDLE;
    VulkanDevice* const _mpDevice = nullptr;
    thread::Mutex* _mpSubmitMutex = nullptr;
    u32 _mFlags                   = QUEUE_FLAG_NONE;
    float _mTimestampPeriod       = 0.0f;
    u32 _mVkQueueFamilyIndex : 8  = U8_MAX;
    u32 _mVkQueueIndex       : 8  = U8_MAX;
    u32 _mGpuMode            : 3  = GPU_MODE_SINGLE;
    u32 _mType               : 3  = MAX_QUEUE_TYPE;
    u32 _mNodeIndex          : 4  = 0;
};

}  // namespace axe::rhi