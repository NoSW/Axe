
#include "02Rhi/Vulkan/VulkanFence.hpp"
#include "02Rhi/Vulkan/VulkanDevice.hpp"
#include <volk/volk.h>

namespace axe::rhi
{

bool VulkanFence::_create(FenceDesc& desc) noexcept
{
    AXE_ASSERT(_mpDevice);
    AXE_ASSERT(_mpDevice->_mpHandle);
    VkFenceCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = (VkFenceCreateFlags)(desc.mIsSignaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0)};

    auto result = vkCreateFence(_mpDevice->_mpHandle, &createInfo, nullptr, &__mpHandle);
    if (VK_FAILED(result)) { AXE_ERROR("Failed to create VulkanFence due to {}", string_VkResult(result)); }
    _mSubmitted = false;
    return __mpHandle != VK_NULL_HANDLE;
}

bool VulkanFence::_destroy() noexcept
{
    AXE_ASSERT(_mpDevice);
    AXE_ASSERT(_mpDevice->_mpHandle);
    AXE_ASSERT(__mpHandle);
    vkDestroyFence(_mpDevice->_mpHandle, __mpHandle, nullptr);
    __mpHandle = VK_NULL_HANDLE;
    return true;
}

void VulkanFence::wait() noexcept
{
    AXE_ASSERT(_mpDevice);
    AXE_ASSERT(_mpDevice->_mpHandle);
    AXE_ASSERT(__mpHandle);
    if (_mSubmitted)
    {
        auto result = vkWaitForFences(_mpDevice->_mpHandle, 1, &__mpHandle, VK_TRUE, U64_MAX);
        if (VK_FAILED(result)) { AXE_ERROR("Failed to wait for fence due to {}", string_VkResult((result))); }
        else { _mSubmitted = false; }
    }
}

FenceStatus VulkanFence::status() noexcept
{
    if (_mSubmitted)
    {
        auto result = vkGetFenceStatus(_mpDevice->_mpHandle, __mpHandle);
        if (result == VK_SUCCESS)
        {
            // fences always have to be reset before they can be used again
            vkResetFences(_mpDevice->_mpHandle, 1, &__mpHandle);
            _mSubmitted = false;
            return FENCE_STATUS_COMPLETE;
        }
        else { return FENCE_STATUS_INCOMPLETE; }
    }
    else { return FENCE_STATUS_NOTSUBMITTED; }
}

}  // namespace axe::rhi