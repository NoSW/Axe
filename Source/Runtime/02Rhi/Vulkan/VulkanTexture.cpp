#include "02Rhi/Vulkan/VulkanTexture.hpp"
#include "02Rhi/Vulkan/VulkanDevice.hpp"

#include "00Core/Math/Math.hpp"

#include <volk.h>
#include <tiny_imageformat/tinyimageformat_apis.h>
#include <tiny_imageformat/tinyimageformat_query.h>

namespace axe::rhi
{

static void get_planar_image_memory_requirement(
    VkDevice device, VkImage image, u32 planesCount, VkMemoryRequirements& outVkMemReq, std::array<u64, MAX_PLANE_COUNT> outPlanesOffsets) noexcept
{
    outVkMemReq.size                                        = 0;
    outVkMemReq.alignment                                   = 0;
    outVkMemReq.memoryTypeBits                              = 0;

    VkImagePlaneMemoryRequirementsInfo imagePlaneMemReqInfo = {VK_STRUCTURE_TYPE_IMAGE_PLANE_MEMORY_REQUIREMENTS_INFO, NULL};

    VkImageMemoryRequirementsInfo2 imagePlaneMemReqInfo2    = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2};
    imagePlaneMemReqInfo2.pNext                             = &imagePlaneMemReqInfo;
    imagePlaneMemReqInfo2.image                             = image;

    VkMemoryDedicatedRequirements memDedicatedReq           = {VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS, NULL};
    VkMemoryRequirements2 memReq2                           = {VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
    memReq2.pNext                                           = &memDedicatedReq;

    for (u32 i = 0; i < planesCount; ++i)
    {
        imagePlaneMemReqInfo.planeAspect = (VkImageAspectFlagBits)(VK_IMAGE_ASPECT_PLANE_0_BIT << i);
        vkGetImageMemoryRequirements2(device, &imagePlaneMemReqInfo2, &memReq2);

        outPlanesOffsets[i] += outVkMemReq.size;
        outVkMemReq.alignment = std::max(memReq2.memoryRequirements.alignment, outVkMemReq.alignment);
        outVkMemReq.size += math::round_up(memReq2.memoryRequirements.size, memReq2.memoryRequirements.alignment);
        outVkMemReq.memoryTypeBits |= memReq2.memoryRequirements.memoryTypeBits;
    }
}

u32 get_memory_type(u32 typeBits, const VkPhysicalDeviceMemoryProperties& memoryProperties, const VkMemoryPropertyFlags& properties) noexcept
{
    for (u32 i = 0; i < memoryProperties.memoryTypeCount; i++)
    {
        if ((typeBits & 1) == 1)
        {
            if ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }
        typeBits >>= 1;
    }

    return 0;
}

bool VulkanTexture::_create(const TextureDesc& desc) noexcept
{
    // args check
    if (desc.mSampleCount > MSAA_SAMPLE_COUNT_1 && desc.mMipLevels > 1)
    {
        AXE_ASSERT(false, "Cannot create a multisampled texture with mipmaps");
        return false;
    }

    if (desc.pNativeHandle && !(desc.mFlags & TEXTURE_CREATION_FLAG_IMPORT_BIT))
    {
        _mOwnsImage = false;
        _mpHandle   = static_cast<VkImage>(const_cast<void*>(desc.pNativeHandle));
    }
    else
    {
        _mOwnsImage = true;
    }

    VkImageUsageFlags additionalFlags = 0;
    if (desc.mStartState & RESOURCE_STATE_RENDER_TARGET)
        additionalFlags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    else if (desc.mStartState & RESOURCE_STATE_DEPTH_WRITE)
        additionalFlags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    VkImageType imageType = VK_IMAGE_TYPE_MAX_ENUM;
    if (desc.mFlags & TEXTURE_CREATION_FLAG_FORCE_2D)
    {
        AXE_ASSERT(desc.mDepth == 1);
        imageType = VK_IMAGE_TYPE_2D;
    }
    else if (desc.mFlags & TEXTURE_CREATION_FLAG_FORCE_3D)
    {
        imageType = VK_IMAGE_TYPE_3D;
    }
    else
    {
        if (desc.mDepth > 1) { imageType = VK_IMAGE_TYPE_3D; }
        else if (desc.mHeight > 1) { imageType = VK_IMAGE_TYPE_2D; }
        else { imageType = VK_IMAGE_TYPE_1D; }
    }

    DescriptorType descriptors = desc.mDescriptors;
    bool cubemapRequired       = (DESCRIPTOR_TYPE_TEXTURE_CUBE == (descriptors & DESCRIPTOR_TYPE_TEXTURE_CUBE));

    const bool isPlanarFormat  = TinyImageFormat_IsPlanar(desc.mFormat);
    const u32 numOfPlanes      = TinyImageFormat_NumOfPlanes(desc.mFormat);
    const bool isSinglePlane   = TinyImageFormat_IsSinglePlane(desc.mFormat);
    AXE_ASSERT(
        ((isSinglePlane && numOfPlanes == 1) || (!isSinglePlane && numOfPlanes > 1 && numOfPlanes <= MAX_PLANE_COUNT)),
        "Number of planes for multi-planar formats must be 2 or 3 and for single-planar formats it must be 1.");

    bool arrayRequired = imageType == VK_IMAGE_TYPE_3D;

    // create image
    if (_mpHandle == VK_NULL_HANDLE)
    {
        VkImageCreateInfo createInfo{
            .sType     = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext     = nullptr,
            .flags     = 0,
            .imageType = imageType,
            .format    = to_vk_enum(desc.mFormat),
            .extent{.width = desc.mWidth, .height = desc.mHeight, .depth = desc.mDepth},
            .mipLevels             = desc.mMipLevels,
            .arrayLayers           = desc.mArraySize,
            .samples               = to_vk_enum(desc.mSampleCount),
            .tiling                = VK_IMAGE_TILING_OPTIMAL,
            .usage                 = (to_image_usage(desc.mDescriptors) | additionalFlags),
            .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices   = nullptr,
            .initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED,
        };

        if (cubemapRequired)
        {
            createInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        }
        if (arrayRequired)
        {
            createInfo.flags |= VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT_KHR;
        }

        VkFormatProperties formatProps{};
        vkGetPhysicalDeviceFormatProperties(_mpDevice->_mpAdapter->handle(), createInfo.format, &formatProps);
        if (isPlanarFormat)
        {
            // multi-planar formats must have each plane separately bound to memory, rather than having a single memory binding for the whole image
            AXE_ASSERT(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DISJOINT_BIT);
            createInfo.flags |= VK_IMAGE_CREATE_DISJOINT_BIT;
        }
        if ((createInfo.usage & VK_IMAGE_USAGE_SAMPLED_BIT) || (createInfo.usage & VK_IMAGE_USAGE_STORAGE_BIT))
        {
            // Make it easy to copy to and from textures
            createInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }

        AXE_ASSERT(_mpDevice->_mpAdapter->isSupportRead(desc.mFormat), "GPU shader can't' read from this format");

        auto formatFeatureFlags = formatProps.optimalTilingFeatures & image_usage_to_format_feature(createInfo.usage);
        AXE_ASSERT(formatFeatureFlags, "Format is not supported for GPU local images (i.e. not host visible images)");

        VmaAllocationCreateInfo memReqs{};
        if (desc.mFlags & TEXTURE_CREATION_FLAG_OWN_MEMORY_BIT) { memReqs.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT; }
        memReqs.usage = (VmaMemoryUsage)VMA_MEMORY_USAGE_GPU_ONLY;

        if (_mpDevice->_mExternalMemoryExtension && desc.mFlags & TEXTURE_CREATION_FLAG_IMPORT_BIT)
        {
            VkExternalMemoryImageCreateInfoKHR externalInfo{
                .sType       = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO_KHR,
                .pNext       = nullptr,
                .handleTypes = (VkExternalMemoryHandleTypeFlagBits)0};

#ifdef VK_USE_PLATFORM_WIN32_KHR
            struct ImportHandleInfo
            {
                void* pHandle;
                VkExternalMemoryHandleTypeFlagBitsKHR mHandleType;
            };

            auto* pHandleInfo = (ImportHandleInfo*)(desc.pNativeHandle);
            VkImportMemoryWin32HandleInfoKHR importInfo{
                .sType      = VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR,
                .pNext      = nullptr,
                .handleType = pHandleInfo->mHandleType,
                .handle     = pHandleInfo->pHandle,
                .name       = L"Texture133"};

            memReqs.pUserData = &importInfo;
            // Allocate external (importable / exportable) memory as dedicated memory to avoid adding unnecessary complexity to the Vulkan Memory Allocator
            memReqs.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

            externalInfo.handleTypes = pHandleInfo->mHandleType;
#endif
            createInfo.pNext = &externalInfo;
        }
        else if (_mpDevice->_mExternalMemoryExtension && desc.mFlags & TEXTURE_CREATION_FLAG_EXPORT_BIT)
        {
            VkExportMemoryAllocateInfoKHR exportMemoryInfo{
                .sType       = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO_KHR,
                .pNext       = nullptr,
                .handleTypes = (VkExternalMemoryHandleTypeFlagBits)0};

#ifdef VK_USE_PLATFORM_WIN32_KHR
            exportMemoryInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR;
#endif
            memReqs.pUserData = &exportMemoryInfo;
            // Allocate external (importable / exportable) memory as dedicated memory to avoid adding unnecessary complexity to the Vulkan Memory Allocator
            memReqs.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        }

        // If lazy allocation is requested, check if the hardware supports it
        bool lazyAllocation = desc.mFlags & TEXTURE_CREATION_FLAG_ON_TILE;
        if (lazyAllocation)
        {
            u32 memoryTypeIndex                 = 0;
            VmaAllocationCreateInfo lazyMemReqs = memReqs;
            lazyMemReqs.usage                   = VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED;
            if (vmaFindMemoryTypeIndex(_mpDevice->_mpVmaAllocator, U32_MAX, &lazyMemReqs, &memoryTypeIndex) == VK_SUCCESS)
            {
                memReqs = lazyMemReqs;
                createInfo.usage |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
                // The Vulkan spec states: If usage includes VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
                // then bits other than VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                // and VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT must not be set
                createInfo.usage &= (VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
                _mLazilyAllocated = true;
            }
        }

        VmaAllocationInfo allocInfo{};
        if (isSinglePlane)
        {
            if (VK_FAILED(vmaCreateImage(_mpDevice->_mpVmaAllocator, &createInfo, &memReqs, &_mpHandle, &_mpVkAllocation, &allocInfo))) { return false; }
        }
        else  // Multi-planar formats
        {
            // Create info requires the mutable format flag set for multi planar images
            // Also pass the format list for mutable formats as per recommendation from the spec
            // Might help to keep DCC enabled if we ever use this as a output format
            // DCC gets disabled when we pass mutable format bit to the create info. Passing the format list helps the driver to enable it
            VkFormat planarFormat                     = to_vk_enum(desc.mFormat);
            VkImageFormatListCreateInfoKHR formatList = {
                .sType           = VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO_KHR,
                .pNext           = nullptr,
                .viewFormatCount = 1,
                .pViewFormats    = &planarFormat

            };

            createInfo.flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
            createInfo.pNext = &formatList;

            // Create Image
            if (VK_FAILED(vkCreateImage(_mpDevice->handle(), &createInfo, nullptr, &_mpHandle))) { return false; }

            VkMemoryRequirements vkMemReq{};
            std::array<u64, MAX_PLANE_COUNT> planesOffsets{};
            get_planar_image_memory_requirement(_mpDevice->handle(), _mpHandle, numOfPlanes, vkMemReq, planesOffsets);

            // Allocate image memory
            VkMemoryAllocateInfo memAllocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
            memAllocInfo.allocationSize       = vkMemReq.size;
            VkPhysicalDeviceMemoryProperties memProps{};
            vkGetPhysicalDeviceMemoryProperties(_mpDevice->_mpAdapter->handle(), &memProps);
            memAllocInfo.memoryTypeIndex = get_memory_type(vkMemReq.memoryTypeBits, memProps, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            if (VK_FAILED(vkAllocateMemory(_mpDevice->handle(), &memAllocInfo, nullptr, &_mpVkDeviceMemory))) { return false; }

            // Bind planes to their memories
            VkBindImageMemoryInfo bindImagesMemoryInfo[MAX_PLANE_COUNT];
            VkBindImagePlaneMemoryInfo bindImagePlanesMemoryInfo[MAX_PLANE_COUNT];
            for (u32 i = 0; i < numOfPlanes; ++i)
            {
                VkBindImagePlaneMemoryInfo& bindImagePlaneMemoryInfo = bindImagePlanesMemoryInfo[i];
                bindImagePlaneMemoryInfo.sType                       = VK_STRUCTURE_TYPE_BIND_IMAGE_PLANE_MEMORY_INFO;
                bindImagePlaneMemoryInfo.pNext                       = nullptr;
                bindImagePlaneMemoryInfo.planeAspect                 = (VkImageAspectFlagBits)(VK_IMAGE_ASPECT_PLANE_0_BIT << i);

                VkBindImageMemoryInfo& bindImageMemoryInfo           = bindImagesMemoryInfo[i];
                bindImageMemoryInfo.sType                            = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO;
                bindImageMemoryInfo.pNext                            = &bindImagePlaneMemoryInfo;
                bindImageMemoryInfo.image                            = _mpHandle;
                bindImageMemoryInfo.memory                           = _mpVkDeviceMemory;
                bindImageMemoryInfo.memoryOffset                     = planesOffsets[i];
            }

            if (VK_FAILED(vkBindImageMemory2(_mpDevice->handle(), numOfPlanes, bindImagesMemoryInfo))) { return false; }
        }

    }  // if (_mpHandle == VK_NULL_HANDLE)

    // Create image view
    VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
    switch (imageType)
    {
        case VK_IMAGE_TYPE_1D: viewType = desc.mArraySize > 1 ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D; break;
        case VK_IMAGE_TYPE_2D:
            if (cubemapRequired) { viewType = desc.mArraySize > 6 ? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY : VK_IMAGE_VIEW_TYPE_CUBE; }
            else { viewType = desc.mArraySize > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D; }
            break;
        case VK_IMAGE_TYPE_3D:
            if (desc.mArraySize > 1) { AXE_ASSERT(false, "Cannot support 3D Texture Array in Vulkan"); }
            viewType = VK_IMAGE_VIEW_TYPE_3D;
            break;
        default: AXE_ASSERT(false, "Image Format not supported!"); break;
    }

    auto imgViewFormat = to_vk_enum(desc.mFormat);
    VkImageViewCreateInfo srvDesc{
        .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext    = nullptr,
        .flags    = 0,
        .image    = _mpHandle,
        .viewType = viewType,
        .format   = imgViewFormat,
        .components{.r = VK_COMPONENT_SWIZZLE_R, .g = VK_COMPONENT_SWIZZLE_G, .b = VK_COMPONENT_SWIZZLE_B, .a = VK_COMPONENT_SWIZZLE_A},
        .subresourceRange{.aspectMask     = determine_aspect_mask(imgViewFormat, false),
                          .baseMipLevel   = 0,
                          .levelCount     = desc.mMipLevels,
                          .baseArrayLayer = 0,
                          .layerCount     = desc.mArraySize}};
    _mAspectMask = determine_aspect_mask(imgViewFormat, true);

    if (false /* desc.pVkSamplerYcbcrConversionInfo*/)
    {
        srvDesc.pNext = nullptr; /* desc.pVkSamplerYcbcrConversionInfo*/
    }

    if (descriptors & DESCRIPTOR_TYPE_TEXTURE)
    {
        if (VK_FAILED(vkCreateImageView(_mpDevice->handle(), &srvDesc, nullptr, &_mpVkSRVDescriptor))) { return false; }
    }
    if ((TinyImageFormat_HasStencil(desc.mFormat)) && (descriptors & DESCRIPTOR_TYPE_TEXTURE))
    {
        srvDesc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
        if (VK_FAILED(vkCreateImageView(_mpDevice->handle(), &srvDesc, nullptr, &_mpVkSRVStencilDescriptor))) { return false; }
    }
    if (descriptors & DESCRIPTOR_TYPE_RW_TEXTURE)
    {
        VkImageViewCreateInfo uavDesc = srvDesc;
        // NOTE : We dont support imageCube, imageCubeArray for consistency with other APIs
        // All cubemaps will be used as image2DArray for Image Load / Store ops
        if (uavDesc.viewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY || uavDesc.viewType == VK_IMAGE_VIEW_TYPE_CUBE)
            uavDesc.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        uavDesc.subresourceRange.levelCount = 1;

        _mpVkUAVDescriptors.resize(desc.mMipLevels, VK_NULL_HANDLE);
        for (u32 i = 0; i < desc.mMipLevels; ++i)
        {
            uavDesc.subresourceRange.baseMipLevel = i;
            if (VK_FAILED(vkCreateImageView(_mpDevice->handle(), &uavDesc, nullptr, &_mpVkUAVDescriptors[i]))) { return false; }
        }
    }

    // populate members of this object
    _mWidth             = desc.mWidth;
    _mHeight            = desc.mHeight;
    _mDepth             = desc.mDepth;
    _mMipLevels         = desc.mMipLevels;
    _mUav               = desc.mDescriptors & DESCRIPTOR_TYPE_RW_TEXTURE;
    _mArraySizeMinusOne = desc.mArraySize - 1;
    _mFormat            = desc.mFormat;
    _mSampleCount       = desc.mSampleCount;

    return true;
}

bool VulkanTexture::_destroy() noexcept
{
    if (_mOwnsImage)
    {
        const TinyImageFormat fmt = (TinyImageFormat)_mFormat;
        const bool isSinglePlane  = TinyImageFormat_IsSinglePlane(fmt);
        if (isSinglePlane)
        {
            vmaDestroyImage(_mpDevice->_mpVmaAllocator, _mpHandle, _mpVkAllocation);
        }
        else
        {
            vkDestroyImage(_mpDevice->handle(), _mpHandle, nullptr);
            vkFreeMemory(_mpDevice->handle(), _mpVkDeviceMemory, nullptr);
        }
    }

    if (_mpVkSRVDescriptor != VK_NULL_HANDLE) { vkDestroyImageView(_mpDevice->handle(), _mpVkSRVDescriptor, nullptr); }
    if (_mpVkSRVStencilDescriptor != VK_NULL_HANDLE) { vkDestroyImageView(_mpDevice->handle(), _mpVkSRVStencilDescriptor, nullptr); }
    for (auto& imgView : _mpVkUAVDescriptors) { vkDestroyImageView(_mpDevice->handle(), imgView, nullptr); }

    // if (_mpSvt)
    // {
    //     removeVirtualTexture(pRenderer, pTexture->pSvt);
    // }

    return true;
}

}  // namespace axe::rhi