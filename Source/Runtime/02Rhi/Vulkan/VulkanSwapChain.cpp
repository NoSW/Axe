#include "02Rhi/Vulkan/VulkanSwapChain.hpp"
#include "02Rhi/Vulkan/VulkanFence.hpp"
#include "02Rhi/Vulkan/VulkanDevice.hpp"

#include "00Core/Window/Window.hpp"

#include <tiny_imageformat/tinyimageformat_apis.h>

#include <volk.h>
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
    if (AXE_FAILED(SDL_Vulkan_CreateSurface(desc.mpWindow->handle(), _mpDevice->_mpAdapter->backendHandle(), &_mpVkSurface) == SDL_TRUE)) { return false; }

    // Find best queue family
    //// Get queue family properties
    u32 presentQuFamIndexCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(_mpDevice->_mpAdapter->handle(), &presentQuFamIndexCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilyProperties(presentQuFamIndexCount);
    vkGetPhysicalDeviceQueueFamilyProperties(_mpDevice->_mpAdapter->handle(), &presentQuFamIndexCount, queueFamilyProperties.data());
    AXE_ASSERT(presentQuFamIndexCount);

    //// find the first dedicated queue and first available queue which supports present
    u32 firstAvailablePresentQuFamIndex = U32_MAX;
    u32 firstDedicatedPresentQuFamIndex = U32_MAX;
    u32 finalPresentQuFamIndex          = U32_MAX;
    for (u32 i = 0; i < queueFamilyProperties.size(); ++i)
    {
        VkBool32 isSupport = false;
        auto result        = vkGetPhysicalDeviceSurfaceSupportKHR(_mpDevice->_mpAdapter->handle(), i, _mpVkSurface, &isSupport);
        if (VK_SUCCEEDED(result) && isSupport == VK_TRUE)
        {
            if (firstAvailablePresentQuFamIndex == U32_MAX)
            {
                firstAvailablePresentQuFamIndex = i;
            }

            if (firstDedicatedPresentQuFamIndex == U32_MAX && ((VulkanQueue*)desc.mpPresentQueue)->_mVkQueueFamilyIndex != i)
            {
                firstDedicatedPresentQuFamIndex = i;
                break;
            }
        }
    }

    //// Choose first dedicated queue firstly, otherwise choose first available one
    if (firstDedicatedPresentQuFamIndex != U32_MAX)
    {
        finalPresentQuFamIndex = firstDedicatedPresentQuFamIndex;
    }
    else if (firstAvailablePresentQuFamIndex != U32_MAX)
    {
        finalPresentQuFamIndex = firstAvailablePresentQuFamIndex;
    }
    else
    {
        AXE_ERROR("No present queue family index for creating swapchain");
        return false;
    }

    VkQueue presentQueueHandle = VK_NULL_HANDLE;
    if (finalPresentQuFamIndex != ((VulkanQueue*)desc.mpPresentQueue)->_mVkQueueFamilyIndex)
    {
        vkGetDeviceQueue(_mpDevice->handle(), finalPresentQuFamIndex, 0, &_mpPresentQueueHandle);
    }

    // Capabilities
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    auto result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_mpDevice->_mpAdapter->handle(), _mpVkSurface, &surfaceCapabilities);
    if (VK_FAILED(result))
    {
        AXE_ERROR("Failed to get surface capabilities, {}", string_VkResult(result));
        return false;
    }
    // Capabilities -> image count
    u32 desiredImageCount = desc.mImageCount == 0 ? std::min(surfaceCapabilities.minImageCount + 1, surfaceCapabilities.maxImageCount) : desc.mImageCount;
    if (desc.mImageCount < surfaceCapabilities.minImageCount)
    {
        AXE_WARN("Requested image count({}) in swapchain is less than minImageCount({})", desc.mImageCount, surfaceCapabilities.minImageCount);
        desiredImageCount = surfaceCapabilities.minImageCount;
    }
    if (desc.mImageCount > surfaceCapabilities.maxImageCount)
    {
        AXE_WARN("Requested image count({}) in swapchain is greater than minImageCount({})", desc.mImageCount, surfaceCapabilities.maxImageCount);
        desiredImageCount = surfaceCapabilities.maxImageCount;
    }

    // Capabilities -> extent
    auto desiredSwapchainExtent = surfaceCapabilities.currentExtent;
    if (surfaceCapabilities.currentExtent.width == -1)
    {
        desiredSwapchainExtent        = {desc.mWidth, desc.mHeight};
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
    if (VK_FAILED(vkGetPhysicalDeviceSurfaceFormatsKHR(_mpDevice->_mpAdapter->handle(), _mpVkSurface, &formatsCount, nullptr)) || (formatsCount == 0))
    {
        AXE_ERROR("Error occurred during presentation surface formats enumeration!");
        return false;
    }
    std::vector<VkSurfaceFormatKHR> surfaceFormats(formatsCount);
    if (VK_FAILED(vkGetPhysicalDeviceSurfaceFormatsKHR(_mpDevice->_mpAdapter->handle(), _mpVkSurface, &formatsCount, surfaceFormats.data())))
    {
        AXE_ERROR("Error occurred during presentation surface formats enumeration!");
        return false;
    }

    constexpr VkSurfaceFormatKHR sRGBSurfaceFormat = {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    constexpr VkSurfaceFormatKHR hdrSurfaceFormat  = {VK_FORMAT_A2B10G10R10_UNORM_PACK32, VK_COLOR_SPACE_HDR10_ST2084_EXT};
    bool isHDRSupported = false, isSRGBSupported = false;
    for (const auto& format : surfaceFormats)
    {
        if (format.format == hdrSurfaceFormat.format && format.colorSpace == hdrSurfaceFormat.colorSpace) { isHDRSupported = true; }
        if (format.format == sRGBSurfaceFormat.format && format.colorSpace == sRGBSurfaceFormat.colorSpace) { isSRGBSupported = true; }
    }
    bool useSRGB = !desc.mUseHDR || (desc.mUseHDR && !isHDRSupported);
    if (desc.mUseHDR && !isHDRSupported) { AXE_WARN("Presentation surface doesn't support hdr format, using sRGB instead"); }
    if (useSRGB && !isSRGBSupported) { AXE_WARN("Presentation surface doesn't support sRGB format, but use it anyway"); }
    auto desiredSurfaceFormat = useSRGB ? sRGBSurfaceFormat : hdrSurfaceFormat;

    // Present Mode, 6 types
    // https://www.intel.com/content/www/us/en/developer/articles/training/api-without-secrets-introduction-to-vulkan-part-2.html
    u32 presentModesCount     = 0;
    if (VK_FAILED(vkGetPhysicalDeviceSurfacePresentModesKHR(_mpDevice->_mpAdapter->handle(), _mpVkSurface, &presentModesCount, nullptr)) || (presentModesCount == 0))
    {
        AXE_ERROR("Error occurred during presentation surface formats enumeration!");
        return false;
    }
    std::vector<VkPresentModeKHR> presentModes(presentModesCount);
    if (VK_FAILED(vkGetPhysicalDeviceSurfacePresentModesKHR(_mpDevice->_mpAdapter->handle(), _mpVkSurface, &presentModesCount, presentModes.data())))
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

    auto oldSwapchain                            = _mpHandle;
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
        .pQueueFamilyIndices   = &finalPresentQuFamIndex,
        .preTransform          = desiredSurfaceTransform,
        .compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode           = desiredMode,
        .clipped               = true,
        .oldSwapchain          = oldSwapchain};

    result = vkCreateSwapchainKHR(_mpDevice->handle(), &swapchainCreateInfo, nullptr, &_mpHandle);
    if (VK_FAILED(result))
    {
        AXE_ERROR("Could not create swap chain, {}", string_VkResult(result));
        return false;
    }
    if (oldSwapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(_mpDevice->handle(), oldSwapchain, nullptr);
    }

    // create render targets from this swapchain
    u32 availableImageCount = 0;
    if (VK_FAILED(vkGetSwapchainImagesKHR(_mpDevice->handle(), _mpHandle, &availableImageCount, nullptr))) { return false; }
    AXE_CHECK(availableImageCount == desiredImageCount);

    std::pmr::vector<VkImage> swapchainImages(availableImageCount);
    if (VK_FAILED(vkGetSwapchainImagesKHR(_mpDevice->handle(), _mpHandle, &availableImageCount, swapchainImages.data()))) { return false; }

    RenderTargetDesc renderTargetDesc = {
        .mWidth     = desc.mWidth,
        .mHeight    = desc.mHeight,
        .mDepth     = 1,
        .mArraySize = 1};
    renderTargetDesc.mFormat          = TinyImageFormat_FromVkFormat((TinyImageFormat_VkFormat)desiredSurfaceFormat.format);
    renderTargetDesc.mClearValue      = desc.mColorClearValue;
    renderTargetDesc.mMSAASampleCount = MSAA_SAMPLE_COUNT_1;
    renderTargetDesc.mSampleQuality   = 0;
    renderTargetDesc.mStartState      = RESOURCE_STATE_PRESENT;

    // Populate the vk_image field and add the Vulkan texture objects
    for (u32 i = 0; i < swapchainImages.size(); ++i)
    {
        char buf[32]{};
        sprintf(buf, "Swapchain RT[%u]", i);
        renderTargetDesc.mpName         = buf;
        renderTargetDesc.mpNativeHandle = (void*)swapchainImages[i];
        // TODO: addRenderTargets
    }

    // populate this object
    _mpPresentQueueHandle     = presentQueueHandle;
    _mEnableVsync             = desc.mEnableVsync;
    _mImageCount              = availableImageCount;
    _mVkSwapchainFormat       = desiredSurfaceFormat.format;
    _mPresentQueueFamilyIndex = finalPresentQuFamIndex;
    return true;
}  // namespace axe::rhi

bool VulkanSwapChain::_destroy() noexcept
{
    // for (uint32_t i = 0; i < pSwapChain->mImageCount; ++i)
    // {
    //     removeRenderTarget(pDevice, pSwapChain->ppRenderTargets[i]);
    // }

    vkDestroySwapchainKHR(_mpDevice->handle(), _mpHandle, nullptr);
    vkDestroySurfaceKHR(_mpDevice->_mpAdapter->backendHandle(), _mpVkSurface, nullptr);
    return true;
}

void VulkanSwapChain::acquireNextImage(Semaphore* pSignalSemaphore, u32& outImageIndex) noexcept
{
    auto* pVulkanSemaphore = (VulkanSemaphore*)pSignalSemaphore;
    auto result            = vkAcquireNextImageKHR(_mpDevice->handle(), _mpHandle, U64_MAX, pVulkanSemaphore->handle(), VK_NULL_HANDLE, &outImageIndex);

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
    auto result        = vkAcquireNextImageKHR(_mpDevice->handle(), _mpHandle, U64_MAX, VK_NULL_HANDLE, pVulkanFence->handle(), &outImageIndex);

    // If swapchain is out of date, let caller know by setting image index to -1
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        outImageIndex = -1;
        vkResetFences(_mpDevice->handle(), 1, &pVulkanFence->_mpHandle);
        pVulkanFence->_mSubmitted = false;
        return;
    }

    pVulkanFence->_mSubmitted = true;
}

}  // namespace axe::rhi