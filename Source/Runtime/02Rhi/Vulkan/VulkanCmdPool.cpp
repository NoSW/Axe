#include "02Rhi/Vulkan/VulkanCmdPool.hpp"
#include "02Rhi/Vulkan/VulkanQueue.hpp"
#include <volk/volk.h>

namespace axe::rhi
{

bool VulkanCmdPool::_create(CmdPoolDesc& desc) noexcept
{
    _mpQueue = (VulkanQueue*)desc.mpUsedForQueue;
    VkCommandPoolCreateInfo createInfo{
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext            = nullptr,
        .flags            = 0,
        .queueFamilyIndex = _mpQueue->_mVkQueueFamilyIndex,
    };

    // VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT: Indicates that command buffers, allocated from this pool, may be reset individually. Normally,
    // without this flag, we can’t rerecord the same command buffer multiple times. It must be reset first. And, what’s more, command buffers created
    // from one pool may be reset only all at once. Specifying this flag allows us to reset command buffers individually, and (even better) it is done
    // implicitly by calling the vkBeginCommandBuffer() function.
    if (desc.mAllowIndividualReset) { createInfo.flags |= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; }

    // VK_COMMAND_POOL_CREATE_TRANSIENT_BIT: This flag tells the driver that command buffers allocated from this pool will be living for a short
    // amount of time, they will be often recorded and reset (re-recorded). This information helps optimize command buffer allocation and perform it
    // more optimally.
    if (desc.mShortLived) { createInfo.flags |= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT; }

    auto result = vkCreateCommandPool(_mpDriver->mpVkDevice, &createInfo, nullptr, &_mpVkCmdPool);
    if (VK_FAILED(result)) { AXE_ERROR("Failed to create VulkanCmdPool due to {}", string_VkResult(result)); }
    return _mpVkCmdPool != VK_NULL_HANDLE;
}

bool VulkanCmdPool::_destroy() noexcept
{
    AXE_ASSERT(_mpDriver && _mpDriver->mpVkDevice && _mpVkCmdPool);
    vkDestroyCommandPool(_mpDriver->mpVkDevice, _mpVkCmdPool, nullptr);
    _mpVkCmdPool = VK_NULL_HANDLE;
    return true;
}

void VulkanCmdPool::reset() noexcept
{
    AXE_ASSERT(_mpDriver && _mpDriver->mpVkDevice && _mpVkCmdPool);
    auto result = vkResetCommandPool(_mpDriver->mpVkDevice, _mpVkCmdPool, 0);
    if (VK_FAILED(result)) { AXE_ERROR("Failed to reset VulkanCmdPool due to   {}", string_VkResult(result)); }
}

}  // namespace axe::rhi