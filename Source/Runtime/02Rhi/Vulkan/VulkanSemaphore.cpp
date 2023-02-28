
#include "02Rhi/Vulkan/VulkanDriver.hpp"
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
    auto result = vkCreateSemaphore(_mpDriver->mpVkDevice, &createInfo, nullptr, &_mpVkSemaphore);
    if (VK_FAILED(result)) { AXE_ERROR("Failed to create VulkanSemaphore due to {}", string_VkResult(result)); }
    _mSignaled = false;
    return _mpVkSemaphore != VK_NULL_HANDLE;
}

bool VulkanSemaphore::_destroy() noexcept
{
    AXE_ASSERT(_mpDriver);
    AXE_ASSERT(_mpDriver->mpVkDevice);
    AXE_ASSERT(_mpVkSemaphore);

    vkDestroySemaphore(_mpDriver->mpVkDevice, _mpVkSemaphore, nullptr);
    _mpVkSemaphore = VK_NULL_HANDLE;

    return true;
}

}  // namespace axe::rhi