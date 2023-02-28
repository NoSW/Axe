#pragma once
#include "02Rhi/Vulkan/VulkanDriver.hpp"
#include "02Rhi/Vulkan/VulkanSemaphore.hpp"

#include "00Core/Thread/Thread.hpp"

namespace axe::thread
{
class Mutex;
}

namespace axe::rhi
{

class VulkanQueue final : public Queue
{
    friend class VulkanDriver;
    friend class VulkanCmdPool;
    friend class VulkanCmd;
    friend class VulkanSwapChain;
    AXE_NON_COPYABLE(VulkanQueue);
    VulkanQueue(VulkanDriver* driver) noexcept : _mpDriver(driver) {}
    bool _create(QueueDesc&) noexcept;
    bool _destroy() noexcept;

public:
    ~VulkanQueue() noexcept override = default;
    void submit(QueueSubmitDesc& desc) noexcept override;
    void present(QueuePresentDesc& desc) noexcept override;
    void waitIdle() noexcept override;

private:
    static void _findQueueFamilyIndex(VulkanDriver* pDriver, u32 nodeIndex, QueueType quType,
                                      VkQueueFamilyProperties2& outQuProps2, u8& outQuFamIndex, u8& outQuIndex) noexcept;

private:
    VkQueue _mpVkQueue            = VK_NULL_HANDLE;
    VulkanDriver* const _mpDriver = nullptr;
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