#pragma once

#include "02Rhi/Rhi.hpp"

#define VK_NO_PROTOTYPES  // used for volk
#include <vulkan/vulkan.h>

namespace axe::rhi
{

class VulkanQueue;
class VulkanCmd;

class VulkanCmdPool final : public CmdPool
{
    friend class VulkanDriver;
    friend class VulkanCmd;
    AXE_NON_COPYABLE(VulkanCmdPool);
    VulkanCmdPool(VulkanDriver* driver) noexcept : _mpDriver(driver) {}
    bool _create(CmdPoolDesc&) noexcept;
    bool _destroy() noexcept;

public:
    ~VulkanCmdPool() noexcept override = default;
    void reset() noexcept override;

private:
    VulkanDriver* const _mpDriver = nullptr;
    VulkanQueue* _mpQueue         = nullptr;
    VkCommandPool _mpVkCmdPool    = VK_NULL_HANDLE;
};

}  // namespace axe::rhi
