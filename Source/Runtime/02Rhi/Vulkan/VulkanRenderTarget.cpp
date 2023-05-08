#include "VulkanRenderTarget.hxx"
#include "VulkanDevice.hxx"
#include "VulkanTexture.hxx"

#include <volk.h>
#include <tiny_imageformat/tinyimageformat_query.h>

namespace axe::rhi
{

bool VulkanRenderTarget::_create(const RenderTargetDesc& desc) noexcept
{
    const bool isDepth = TinyImageFormat_IsDepthOnly(desc.mFormat) || TinyImageFormat_IsDepthAndStencil(desc.mFormat);
    AXE_ASSERT(!((isDepth) && (desc.mDescriptors & DESCRIPTOR_TYPE_RW_TEXTURE)) && "Cannot use depth stencil as UAV");

    _mMipLevels = std::max(1u, desc.mMipLevels);  // clamp mipLevels
    TextureDesc texDesc{
        .pNativeHandle  = desc.mpNativeHandle,
        .pName          = desc.mpName,
        .mClearValue    = desc.mClearValue,
        .mFlags         = desc.mFlags,
        .mWidth         = desc.mWidth,
        .mHeight        = desc.mHeight,
        .mDepth         = desc.mDepth,
        .mArraySize     = desc.mArraySize,
        .mMipLevels     = _mMipLevels,
        .mSampleCount   = desc.mMSAASampleCount,
        .mSampleQuality = desc.mSampleQuality,
        .mFormat        = desc.mFormat,
        .mStartState    = isDepth ? RESOURCE_STATE_RENDER_TARGET : RESOURCE_STATE_DEPTH_WRITE,
        .mDescriptors   = desc.mDescriptors,

    };

    // Create SRV by default for a render target unless this is on tile texture where SRV is not supported
    if (!((u32)desc.mDescriptors & (u32)TEXTURE_CREATION_FLAG_ON_TILE))
    {
        texDesc.mDescriptors |= DESCRIPTOR_TYPE_TEXTURE;
    }
    else
    {
        if (desc.mDescriptors & DESCRIPTOR_TYPE_TEXTURE ||
            desc.mDescriptors & DESCRIPTOR_TYPE_RW_TEXTURE)
        {
            AXE_WARN("On tile textures do not support DESCRIPTOR_TYPE_TEXTURE or DESCRIPTOR_TYPE_RW_TEXTURE");
        }
        // On tile textures do not support SRV/UAV as there is no backing memory
        // You can only read these textures as input attachments inside same render pass
        texDesc.mDescriptors |= (~(DESCRIPTOR_TYPE_TEXTURE | DESCRIPTOR_TYPE_RW_TEXTURE));
    }

    if (isDepth)
    {
        const auto depthStencilFormat = to_vk_enum(desc.mFormat);
        if (depthStencilFormat != VK_FORMAT_UNDEFINED)
        {
            VkImageFormatProperties2 formatProperties{};
            VkPhysicalDeviceImageFormatInfo2 formatInfo{
                .sType  = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2,
                .pNext  = nullptr,
                .format = depthStencilFormat,
                .type   = VK_IMAGE_TYPE_2D,
                .tiling = VK_IMAGE_TILING_OPTIMAL,
                .usage  = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                .flags  = 0,

            };
            auto result = vkGetPhysicalDeviceImageFormatProperties2(_mpDevice->_mpAdapter->handle(), &formatInfo, &formatProperties);
            if (VK_FAILED(result))
            {
                texDesc.mFormat |= TinyImageFormat_D16_UNORM;
                AXE_WARN("Depth stencil format ({}) not supported. Falling back to D16 format", reflection::enum_name(desc.mFormat));
            }
        }
    }

    _mpTexture = static_cast<VulkanTexture*>(_mpDevice->createTexture(texDesc));
    if (!_mpTexture)
    {
        AXE_ERROR("Failed to create render target texture");
        return false;
    }

    const u32 depthArraySize = desc.mArraySize * desc.mDepth;
    const auto format        = to_vk_enum(texDesc.mFormat);
    const auto viewType      = (VkImageViewType)(desc.mHeight > 1 ? (depthArraySize > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D) :
                                                                    (depthArraySize > 1 ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D));
    VkImageViewCreateInfo rtvCreateInfo{
        .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext    = nullptr,
        .flags    = 0,
        .image    = _mpTexture->handle(),
        .viewType = viewType,
        .format   = format,
        .components{
            .r = VK_COMPONENT_SWIZZLE_R,
            .g = VK_COMPONENT_SWIZZLE_G,
            .b = VK_COMPONENT_SWIZZLE_B,
            .a = VK_COMPONENT_SWIZZLE_A

        },
        .subresourceRange{
            .aspectMask     = determine_aspect_mask(format, true),
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = depthArraySize

        },
    };

    if (VK_FAILED(vkCreateImageView(_mpDevice->handle(), &rtvCreateInfo, nullptr, &_mpVkDescriptor))) { return false; }

    const bool isTexture = desc.mDescriptors & DESCRIPTOR_TYPE_TEXTURE || desc.mDescriptors & DESCRIPTOR_TYPE_RW_TEXTURE;
    for (u32 i = 0; i < _mMipLevels; ++i)
    {
        rtvCreateInfo.subresourceRange.baseMipLevel = i;
        VkImageView pImageView                      = VK_NULL_HANDLE;
        if (isTexture)
        {
            for (u32 j = 0; j < depthArraySize; ++j)
            {
                rtvCreateInfo.subresourceRange.layerCount     = 1;
                rtvCreateInfo.subresourceRange.baseArrayLayer = j;
                if (VK_FAILED(vkCreateImageView(_mpDevice->handle(), &rtvCreateInfo, nullptr, &pImageView))) { return false; }
            }
        }
        else
        {
            if (VK_FAILED(vkCreateImageView(_mpDevice->handle(), &rtvCreateInfo, nullptr, &pImageView))) { return false; }
        }
        _mpVkSliceDescriptors.push_back(pImageView);
    }

    //
    static std::atomic<u32> sRenderTargetIdCounter = 0;
    _mId                                           = sRenderTargetIdCounter++;
    _mWidth                                        = desc.mWidth;
    _mHeight                                       = desc.mHeight;
    _mArraySize                                    = desc.mArraySize;
    _mDepth                                        = desc.mDepth;
    _mSampleQuality                                = desc.mSampleQuality;
    _mSampleCount                                  = desc.mMSAASampleCount;
    _mFormat                                       = desc.mFormat;
    _mClearValue                                   = desc.mClearValue;

    // Unlike DX12, Vulkan textures start in undefined layout.
    // To keep in line with DX12, we transition them to the specified layout manually so app code doesn't
    // have to worry about this Render targets wont be created during runtime so this overhead will be minimal
    _mpDevice->initial_transition((Texture*)_mpTexture, desc.mStartState);

    return true;
}

bool VulkanRenderTarget::_destroy() noexcept
{
    AXE_ASSERT(_mpDevice);
    auto* pTexture = (Texture*)_mpTexture;
    if (AXE_FAILED(_mpDevice->destroyTexture(pTexture))) { return false; }
    vkDestroyImageView(_mpDevice->handle(), _mpVkDescriptor, nullptr);
    _mpVkDescriptor = VK_NULL_HANDLE;

    for (VkImageView& img : _mpVkSliceDescriptors)
    {
        vkDestroyImageView(_mpDevice->handle(), img, nullptr);
    }
    return true;
}

}  // namespace axe::rhi