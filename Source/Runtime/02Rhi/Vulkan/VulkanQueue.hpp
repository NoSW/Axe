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
class VulkanSwapChain;
class VulkanQueue final : public Queue
{
    friend class VulkanDevice;
    friend class VulkanCmdPool;
    friend class VulkanCmd;
    friend class VulkanSwapChain;
    AXE_NON_COPYABLE(VulkanQueue);
    VulkanQueue(VulkanDevice* device) noexcept : _mpDevice(device) {}
    bool _create(const QueueDesc&) noexcept;
    bool _destroy() noexcept;

public:
    AXE_PUBLIC ~VulkanQueue() noexcept override = default;
    AXE_PUBLIC void submit(QueueSubmitDesc& desc) noexcept override;
    AXE_PUBLIC void present(QueuePresentDesc& desc) noexcept override;
    AXE_PUBLIC void waitIdle() noexcept override;

public:
    auto handle() noexcept { return _mpHandle; }

public:
    constexpr static VkObjectType getVkTypeId() noexcept { return VK_OBJECT_TYPE_QUEUE; }

private:
    VkQueue _mpHandle             = VK_NULL_HANDLE;
    VulkanDevice* const _mpDevice = nullptr;
    thread::Mutex* _mpSubmitMutex = nullptr;
    u32 _mFlags                   = QUEUE_FLAG_NONE;
    float _mTimestampPeriod       = 0.0f;
    u32 _mVkQueueFamilyIndex : 8  = U8_MAX;
    u32 _mVkQueueIndex       : 8  = U8_MAX;
    u32 _mType               : 3  = QUEUE_TYPE_MAX;
};

}  // namespace axe::rhi