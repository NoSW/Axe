
#include "02Rhi/Vulkan/VulkanDevice.hpp"
#include "02Rhi/Vulkan/VulkanSemaphore.hpp"
#include <volk/volk.h>

namespace axe::rhi
{

bool VulkanSemaphore::_create(SemaphoreDesc& desc) noexcept
{
    VkSemaphoreCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
    };
    auto result = vkCreateSemaphore(_mpDevice->handle(), &createInfo, nullptr, &_mpHandle);
    if (VK_FAILED(result)) { AXE_ERROR("Failed to create VulkanSemaphore due to {}", string_VkResult(result)); }
    _mSignaled = false;
    return _mpHandle != VK_NULL_HANDLE;
}

bool VulkanSemaphore::_destroy() noexcept
{
    AXE_ASSERT(_mpDevice);
    AXE_ASSERT(_mpDevice->handle());
    AXE_ASSERT(_mpHandle);

    vkDestroySemaphore(_mpDevice->handle(), _mpHandle, nullptr);
    _mpHandle = VK_NULL_HANDLE;

    return true;
}

}  // namespace axe::rhi