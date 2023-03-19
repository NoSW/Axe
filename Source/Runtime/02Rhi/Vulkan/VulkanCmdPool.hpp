#pragma once

#include "02Rhi/Rhi.hpp"
#include "02Rhi/Vulkan/VulkanEnums.hpp"

namespace axe::rhi
{

class VulkanQueue;
class VulkanCmd;
class VulkanDevice;

class VulkanCmdPool final : public CmdPool
{
    friend class VulkanDevice;
    friend class VulkanCmd;
    AXE_NON_COPYABLE(VulkanCmdPool);
    VulkanCmdPool(VulkanDevice* device) noexcept : _mpDevice(device) {}
    bool _create(CmdPoolDesc&) noexcept;
    bool _destroy() noexcept;

public:
    AXE_PUBLIC ~VulkanCmdPool() noexcept override = default;
    AXE_PUBLIC void reset() noexcept override;

public:
    auto handle() noexcept { return _mpHandle; }

public:
    constexpr static VkObjectType TYPE_ID = VK_OBJECT_TYPE_COMMAND_POOL;

private:
    VulkanDevice* const _mpDevice = nullptr;
    VulkanQueue* _mpQueue         = nullptr;
    VkCommandPool _mpHandle       = VK_NULL_HANDLE;
};

}  // namespace axe::rhi
