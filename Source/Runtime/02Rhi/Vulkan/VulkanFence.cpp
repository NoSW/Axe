
#include "VulkanFence.hxx"
#include "VulkanDevice.hxx"
#include <volk.h>

namespace axe::rhi
{

bool VulkanFence::_create(const FenceDesc& desc) noexcept
{
    AXE_ASSERT(_mpDevice);
    AXE_ASSERT(_mpDevice->handle());
    VkFenceCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = (VkFenceCreateFlags)(desc.mIsSignaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0)};

    auto result = vkCreateFence(_mpDevice->handle(), &createInfo, nullptr, &_mpHandle);
    if (VK_FAILED(result)) { AXE_ERROR("Failed to create VulkanFence due to {}", string_VkResult(result)); }
    _mSubmitted = false;
    return _mpHandle != VK_NULL_HANDLE;
}

bool VulkanFence::_destroy() noexcept
{
    AXE_ASSERT(_mpDevice);
    AXE_ASSERT(_mpDevice->handle());
    AXE_ASSERT(_mpHandle);
    vkDestroyFence(_mpDevice->handle(), _mpHandle, nullptr);
    _mpHandle = VK_NULL_HANDLE;
    return true;
}

void VulkanFence::wait() noexcept
{
    AXE_ASSERT(_mpDevice);
    AXE_ASSERT(_mpDevice->handle());
    AXE_ASSERT(_mpHandle);
    if (_mSubmitted)
    {
        auto result = vkWaitForFences(_mpDevice->handle(), 1, &_mpHandle, VK_TRUE, U64_MAX);
        if (VK_FAILED(result)) { AXE_ERROR("Failed to wait for fence due to {}", string_VkResult((result))); }
        else
        {
            vkResetFences(_mpDevice->handle(), 1, &_mpHandle);
            _mSubmitted = false;
        }
    }
}

FenceStatus VulkanFence::status() noexcept
{
    if (_mSubmitted)
    {
        auto result = vkGetFenceStatus(_mpDevice->handle(), _mpHandle);
        if (result == VK_SUCCESS)
        {
            // fences always have to be reset before they can be used again
            vkResetFences(_mpDevice->handle(), 1, &_mpHandle);
            _mSubmitted = false;
            return FENCE_STATUS_COMPLETE;
        }
        else { return FENCE_STATUS_INCOMPLETE; }
    }
    else { return FENCE_STATUS_NOTSUBMITTED; }
}

}  // namespace axe::rhi