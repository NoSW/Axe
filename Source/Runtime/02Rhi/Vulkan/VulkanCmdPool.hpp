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
    ~VulkanCmdPool() noexcept override = default;
    void reset() noexcept override;

private:
    VulkanDevice* const _mpDevice = nullptr;
    VulkanQueue* _mpQueue         = nullptr;
    VkCommandPool _mpHandle       = VK_NULL_HANDLE;
};

}  // namespace axe::rhi
