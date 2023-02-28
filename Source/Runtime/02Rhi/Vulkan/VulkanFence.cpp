
#include "02Rhi/Vulkan/VulkanFence.hpp"
#include "02Rhi/Vulkan/VulkanDriver.hpp"
#include <volk/volk.h>

namespace axe::rhi
{

bool VulkanFence::_create(FenceDesc& desc) noexcept
{
    AXE_ASSERT(_mpDriver);
    AXE_ASSERT(_mpDriver->mpVkDevice);
    VkFenceCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = (VkFenceCreateFlags)(desc.mIsSignaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0)};

    auto result = vkCreateFence(_mpDriver->mpVkDevice, &createInfo, nullptr, &_mpVkFence);
    if (VK_FAILED(result)) { AXE_ERROR("Failed to create VulkanFence due to {}", string_VkResult(result)); }
    _mSubmitted = false;
    return _mpVkFence != VK_NULL_HANDLE;
}

bool VulkanFence::_destroy() noexcept
{
    AXE_ASSERT(_mpDriver);
    AXE_ASSERT(_mpDriver->mpVkDevice);
    AXE_ASSERT(_mpVkFence);
    vkDestroyFence(_mpDriver->mpVkDevice, _mpVkFence, nullptr);
    _mpVkFence = VK_NULL_HANDLE;
    return true;
}

void VulkanFence::wait() noexcept
{
    AXE_ASSERT(_mpDriver);
    AXE_ASSERT(_mpDriver->mpVkDevice);
    AXE_ASSERT(_mpVkFence);
    if (_mSubmitted)
    {
        auto result = vkWaitForFences(_mpDriver->mpVkDevice, 1, &_mpVkFence, VK_TRUE, U64_MAX);
        if (VK_FAILED(result)) { AXE_ERROR("Failed to wait for fence due to {}", string_VkResult((result))); }
        else { _mSubmitted = false; }
    }
}

FenceStatus VulkanFence::status() noexcept
{
    if (_mSubmitted)
    {
        auto result = vkGetFenceStatus(_mpDriver->mpVkDevice, _mpVkFence);
        if (result == VK_SUCCESS)
        {
            // fences always have to be reset before they can be used again
            vkResetFences(_mpDriver->mpVkDevice, 1, &_mpVkFence);
            _mSubmitted = false;
            return FENCE_STATUS_COMPLETE;
        }
        else { return FENCE_STATUS_INCOMPLETE; }
    }
    else { return FENCE_STATUS_NOTSUBMITTED; }
}

}  // namespace axe::rhi