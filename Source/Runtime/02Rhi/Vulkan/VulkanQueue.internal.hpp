#pragma once

#include "02Rhi/Rhi.hpp"
#include "VulkanEnums.internal.hpp"

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
    explicit VulkanQueue(VulkanDevice* pDevice) noexcept : _mpDevice(pDevice) {}
    bool _create(const QueueDesc&) noexcept;
    bool _destroy() noexcept;

public:
    ~VulkanQueue() noexcept override = default;
    void submit(QueueSubmitDesc& desc) noexcept override;
    void present(QueuePresentDesc& desc) noexcept override;
    void waitIdle() noexcept override;

public:
    AXE_PRIVATE auto handle() noexcept { return _mpHandle; }

public:
    AXE_PRIVATE constexpr static auto VK_TYPE_ID = VK_OBJECT_TYPE_QUEUE;

private:
    VkQueue _mpHandle             = VK_NULL_HANDLE;
    VulkanDevice* const _mpDevice = nullptr;
    thread::Mutex* _mpSubmitMutex = nullptr;
    u32 _mFlags                   = (u32)QueueFlag::NONE;
    float _mTimestampPeriod       = 0.0f;
    u32 _mVkQueueFamilyIndex : 8  = U8_MAX;
    u32 _mVkQueueIndex       : 8  = U8_MAX;
    u32 _mType               : 3  = (u32)QueueTypeFlag::UNDEFINED;
};

}  // namespace axe::rhi