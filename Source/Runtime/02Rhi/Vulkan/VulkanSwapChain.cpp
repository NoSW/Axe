#include "02Rhi/Vulkan/VulkanSwapChain.hpp"
#include "02Rhi/Vulkan/VulkanFence.hpp"

#include "00Core/Window/Window.hpp"

#include <volk/volk.h>
#include <SDL_vulkan.h>

namespace axe::rhi
{
// Since Vulkan is OS-agnostic, and can be used for many different purposes,
// such as math ops, physics calculations, video stream, it's not obvious to
// display an image on the screen in our application's window. Therefore
// swapchain is an extension, not core API, which need to be created ourselves.
// Also, implementation of swapchain is dependent on OS
bool VulkanSwapChain::_create(SwapChainDesc& desc) noexcept
{
    AXE_CHECK(SDL_Vulkan_CreateSurface(desc.mpWindow->handle(), _mpDriver->mpVkInstance, &_mpVkSurface) == SDL_TRUE);
    _mImageCount               = desc.mImageCount;
    _mEnableVsync              = desc.mEnableVsync;

    // Find best queue family
    //// Get queue family properties
    u32 presentQuFamIndexCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(_mpDriver->mpVkActiveGPU, &presentQuFamIndexCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilyProperties(presentQuFamIndexCount);
    vkGetPhysicalDeviceQueueFamilyProperties(_mpDriver->mpVkActiveGPU, &presentQuFamIndexCount, queueFamilyProperties.data());
    AXE_ASSERT(presentQuFamIndexCount);

    //// find the first dedicated queue and first available queue which supports present
    u32 firstAvailablePresentQuFamIndex = U32_MAX;
    u32 firstDedicatedPresentQuFamIndex = U32_MAX;
    for (u32 i = 0; i < queueFamilyProperties.size(); ++i)
    {
        VkBool32 isSupport = false;
        auto result        = vkGetPhysicalDeviceSurfaceSupportKHR(_mpDriver->mpVkActiveGPU, i, _mpVkSurface, &isSupport);
        if (VK_SUCCEEDED(result) && isSupport == VK_TRUE)
        {
            if (firstAvailablePresentQuFamIndex == U32_MAX)
            {
                firstAvailablePresentQuFamIndex = i;
            }

            if (firstDedicatedPresentQuFamIndex == U32_MAX &&
                std::find_if(desc.mpPresentQueues.begin(), desc.mpPresentQueues.end(), [i](auto* qu)
                             { return ((VulkanQueue*)qu)->_mVkQueueFamilyIndex == i; }) == desc.mpPresentQueues.end())
            {
                firstDedicatedPresentQuFamIndex = i;
                break;
            }
        }
    }

    //// Choose first dedicated queue firstly, otherwise choose first available one
    if (firstDedicatedPresentQuFamIndex != U32_MAX) { _mPresentQueueFamilyIndex = firstDedicatedPresentQuFamIndex; }
    else if (firstAvailablePresentQuFamIndex != U32_MAX) { _mPresentQueueFamilyIndex = firstAvailablePresentQuFamIndex; }
    else
    {
        AXE_ERROR("No present queue family index for creating swapchain");
        return false;
    }

    // Capabilities
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    auto result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_mpDriver->mpVkActiveGPU, _mpVkSurface, &surfaceCapabilities);
    if (VK_FAILED(result))
    {
        AXE_ERROR("Failed to get surface capabilities, {}", string_VkResult(result));
        return false;
    }
    // Capabilities -> image count
    u32 desiredImageCount       = std::min(surfaceCapabilities.minImageCount + 1, surfaceCapabilities.maxImageCount);
    // Capabilities -> extent
    auto desiredSwapchainExtent = surfaceCapabilities.currentExtent;
    if (surfaceCapabilities.currentExtent.width == -1)
    {
        desiredSwapchainExtent        = {640, 800};
        desiredSwapchainExtent.width  = std::clamp(desiredSwapchainExtent.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
        desiredSwapchainExtent.height = std::clamp(desiredSwapchainExtent.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
    }
    // Capabilities -> usage flags
    auto desiredUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if ((surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) == 0 ||
        (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) == 0)
    {
        AXE_ERROR("Failed to supported surface attachment");
        return false;
    }
    // Capabilities -> transform
    auto desiredSurfaceTransform = (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) ?
                                       VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR :
                                       surfaceCapabilities.currentTransform;

    // Format
    u32 formatsCount             = 0;
    if (VK_FAILED(vkGetPhysicalDeviceSurfaceFormatsKHR(_mpDriver->mpVkActiveGPU, _mpVkSurface, &formatsCount, nullptr)) || (formatsCount == 0))
    {
        AXE_ERROR("Error occurred during presentation surface formats enumeration!");
        return false;
    }
    std::vector<VkSurfaceFormatKHR> surfaceFormats(formatsCount);
    if (VK_FAILED(vkGetPhysicalDeviceSurfaceFormatsKHR(_mpDriver->mpVkActiveGPU, _mpVkSurface, &formatsCount, surfaceFormats.data())))
    {
        AXE_ERROR("Error occurred during presentation surface formats enumeration!");
        return false;
    }
    auto desiredSurfaceFormat = surfaceFormats[0];
    for (const auto& format : surfaceFormats)
    {
        if (format.format == VK_FORMAT_R8G8B8A8_UNORM)
        {
            desiredSurfaceFormat = format;
            break;
        }
    }

    // Present Mode, 6 types
    // https://www.intel.com/content/www/us/en/developer/articles/training/api-without-secrets-introduction-to-vulkan-part-2.html
    u32 presentModesCount = 0;
    if (VK_FAILED(vkGetPhysicalDeviceSurfacePresentModesKHR(_mpDriver->mpVkActiveGPU, _mpVkSurface, &presentModesCount, nullptr)) || (presentModesCount == 0))
    {
        AXE_ERROR("Error occurred during presentation surface formats enumeration!");
        return false;
    }
    std::vector<VkPresentModeKHR> presentModes(presentModesCount);
    if (VK_FAILED(vkGetPhysicalDeviceSurfacePresentModesKHR(_mpDriver->mpVkActiveGPU, _mpVkSurface, &presentModesCount, presentModes.data())))
    {
        AXE_ERROR("Error occurred during presentation surface formats enumeration!");
        return false;
    }
    VkPresentModeKHR desiredMode = VK_PRESENT_MODE_MAX_ENUM_KHR;
    for (const auto& mode : presentModes)
    {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            desiredMode = mode;
            break;
        }
        else if (mode == VK_PRESENT_MODE_FIFO_KHR) { desiredMode = mode; }
    }
    if (desiredMode == VK_PRESENT_MODE_MAX_ENUM_KHR)
    {
        AXE_ERROR("FIFO present mode is not supported by the swap chain!");
        return false;
    }

    auto oldSwapchain                            = _mpVkSwapChain;
    _mVkSwapchainFormat                          = desiredSurfaceFormat.format;
    VkSwapchainCreateInfoKHR swapchainCreateInfo = {
        .sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext                 = nullptr,
        .flags                 = 0,
        .surface               = _mpVkSurface,
        .minImageCount         = desiredImageCount,
        .imageFormat           = desiredSurfaceFormat.format,
        .imageColorSpace       = desiredSurfaceFormat.colorSpace,
        .imageExtent           = desiredSwapchainExtent,
        .imageArrayLayers      = 1,
        .imageUsage            = (VkImageUsageFlags)desiredUsageFlags,
        .imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices   = &_mPresentQueueFamilyIndex,
        .preTransform          = desiredSurfaceTransform,
        .compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode           = desiredMode,
        .clipped               = true,
        .oldSwapchain          = oldSwapchain};

    result = vkCreateSwapchainKHR(_mpDriver->mpVkDevice, &swapchainCreateInfo, nullptr, &_mpVkSwapChain);
    if (VK_FAILED(result))
    {
        AXE_ERROR("Could not create swap chain, {}", string_VkResult(result));
        return false;
    }
    if (oldSwapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(_mpDriver->mpVkDevice, oldSwapchain, nullptr);
    }
#if AXE_(false, "editing code, not ready yet")
#endif
    return true;
}

bool VulkanSwapChain::_destroy() noexcept
{
    // for (uint32_t i = 0; i < pSwapChain->mImageCount; ++i)
    // {
    //     removeRenderTarget(pDriver, pSwapChain->ppRenderTargets[i]);
    // }

    vkDestroySwapchainKHR(_mpDriver->mpVkDevice, _mpVkSwapChain, nullptr);
    vkDestroySurfaceKHR(_mpDriver->mpVkInstance, _mpVkSurface, nullptr);
    return true;
}

void VulkanSwapChain::acquireNextImage(Semaphore* pSignalSemaphore, u32& outImageIndex) noexcept
{
    auto* pVulkanSemaphore = (VulkanSemaphore*)pSignalSemaphore;
    auto result            = vkAcquireNextImageKHR(_mpDriver->mpVkDevice, _mpVkSwapChain, U64_MAX, pVulkanSemaphore->_mpVkSemaphore, VK_NULL_HANDLE, &outImageIndex);

    // Commonly returned immediately following swapchain resize.
    // Vulkan spec states that this return value constitutes a successful call to vkAcquireNextImageKHR
    // https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/vkAcquireNextImageKHR.html
    switch (result)
    {
        case VK_SUBOPTIMAL_KHR: AXE_INFO("vkAcquireNextImageKHR returned VK_SUBOPTIMAL_KHR. If window was just resized, ignore this message.");
        case VK_SUCCESS: pVulkanSemaphore->_mSignaled = true; break;
        case VK_ERROR_OUT_OF_DATE_KHR:
            outImageIndex                = -1;  // If swapchain is out of date, let caller know by setting image index to -1
            pVulkanSemaphore->_mSignaled = false;
            break;
        default:
            AXE_ERROR("Problem occurred during swap chain image acquisition, {}", string_VkResult(result));
            break;
    }
}

void VulkanSwapChain::acquireNextImage(Fence* pFence, u32& outImageIndex) noexcept
{
    auto* pVulkanFence = (VulkanFence*)pFence;
    auto result        = vkAcquireNextImageKHR(_mpDriver->mpVkDevice, _mpVkSwapChain, U64_MAX, VK_NULL_HANDLE, pVulkanFence->_mpVkFence, &outImageIndex);

    // If swapchain is out of date, let caller know by setting image index to -1
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        outImageIndex = -1;
        vkResetFences(_mpDriver->mpVkDevice, 1, &pVulkanFence->_mpVkFence);
        pVulkanFence->_mSubmitted = false;
        return;
    }

    pVulkanFence->_mSubmitted = true;
}

}  // namespace axe::rhi