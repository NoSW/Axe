#include "VulkanRenderTarget.internal.hpp"
#include "VulkanDevice.internal.hpp"
#include "VulkanTexture.internal.hpp"

#include <volk.h>
#include <tiny_imageformat/tinyimageformat_query.h>

namespace axe::rhi
{

bool VulkanRenderTarget::_create(const RenderTargetDesc& desc) noexcept
{
    const bool isDepth = TinyImageFormat_IsDepthOnly(desc.format) || TinyImageFormat_IsDepthAndStencil(desc.format);
    AXE_ASSERT(!((isDepth) && ((bool)(desc.descriptorType & DescriptorTypeFlag::RW_TEXTURE))) && "Cannot use depth stencil as UAV");

    _mMipLevels = std::max(1u, desc.mipLevels);  // clamp mipLevels
    TextureDesc texDesc{
        .pNativeHandle  = desc.pNativeHandle,
        .pName          = desc.name,
        .clearValue     = desc.clearValue,
        .flags          = desc.flags,
        .width          = desc.width,
        .height         = desc.height,
        .depth          = desc.depth,
        .arraySize      = desc.arraySize,
        .mipLevels      = _mMipLevels,
        .sampleCount    = desc.mMSAASampleCount,
        .sampleQuality  = desc.sampleQuality,
        .format         = desc.format,
        .startState     = isDepth ? ResourceStateFlags::DEPTH_WRITE : ResourceStateFlags::RENDER_TARGET,
        .descriptorType = desc.descriptorType,

    };

    // Create SRV by default for a render target unless this is on tile texture where SRV is not supported
    if (!((u32)desc.flags & (u32)TextureCreationFlags::ON_TILE))
    {
        texDesc.descriptorType |= DescriptorTypeFlag::TEXTURE;
    }
    else
    {
        if ((desc.descriptorType & DescriptorTypeFlag::TEXTURE) || (desc.descriptorType & DescriptorTypeFlag::RW_TEXTURE))
        {
            AXE_WARN("On tile textures do not support DescriptorTypeFlag::TEXTURE or DescriptorTypeFlag::RW_TEXTURE");
        }
        // On tile textures do not support SRV/UAV as there is no backing memory
        // You can only read these textures as input attachments inside same render pass
        texDesc.descriptorType |= (~(DescriptorTypeFlag::TEXTURE | DescriptorTypeFlag::RW_TEXTURE));
    }

    if (isDepth)
    {
        const auto depthStencilFormat = to_vk_enum(desc.format);
        if (depthStencilFormat != VK_FORMAT_UNDEFINED)
        {
            VkImageFormatProperties2 formatProperties{.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2, .pNext = nullptr};
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
                texDesc.format |= TinyImageFormat_D16_UNORM;
                AXE_WARN("Depth stencil format ({}) not supported. Falling back to D16 format", reflection::enum_name(desc.format));
            }
        }
    }

    _mpTexture = static_cast<VulkanTexture*>(_mpDevice->createTexture(texDesc));
    if (!_mpTexture)
    {
        AXE_ERROR("Failed to create render target texture");
        return false;
    }

    const u32 depthArraySize = desc.arraySize * desc.depth;
    const auto format        = to_vk_enum(texDesc.format);
    const auto viewType      = (VkImageViewType)(desc.height > 1 ? (depthArraySize > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D) :
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

    const bool isTexture = (desc.descriptorType & DescriptorTypeFlag::TEXTURE) || (desc.descriptorType & DescriptorTypeFlag::RW_TEXTURE);
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
    _mWidth                                        = desc.width;
    _mHeight                                       = desc.height;
    _mArraySize                                    = desc.arraySize;
    _mDepth                                        = desc.depth;
    _mSampleQuality                                = desc.sampleQuality;
    _mSampleCount                                  = desc.mMSAASampleCount;
    _mFormat                                       = desc.format;
    _mClearValue                                   = desc.clearValue;

    // Unlike DX12, Vulkan textures start in undefined layout.
    // To keep in line with DX12, we transition them to the specified layout manually so app code doesn't
    // have to worry about this Render targets wont be created during runtime so this overhead will be minimal
    _mpDevice->initial_transition((Texture*)_mpTexture, desc.startState);

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