#include "02Rhi/Vulkan/VulkanSampler.hpp"
#include "02Rhi/Vulkan/VulkanDevice.hpp"
#include "02Rhi/Vulkan/VulkanEnums.hpp"
#include <volk/volk.h>

#include <tiny_imageformat/tinyimageformat_apis.h>

namespace axe::rhi
{

bool VulkanSampler::_create(SamplerDesc& desc) noexcept
{
    // default lod values
    float minSamplerLod = 0;
    float maxSamplerLod = desc.mMipMapMode == MIPMAP_MODE_LINEAR ? VK_LOD_CLAMP_NONE : 0;

    // user provided lod values
    if (desc.mSetLodRange)
    {
        minSamplerLod = desc.mMinLod;
        maxSamplerLod = desc.mMaxLod;
    }

    VkSamplerCreateInfo createInfo{
        .sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext                   = nullptr,
        .flags                   = 0,
        .magFilter               = to_vk(desc.mMagFilter),
        .minFilter               = to_vk(desc.mMinFilter),
        .mipmapMode              = to_vk(desc.mMipMapMode),
        .addressModeU            = to_vk(desc.mAddressU),
        .addressModeV            = to_vk(desc.mAddressV),
        .addressModeW            = to_vk(desc.mAddressW),
        .mipLodBias              = desc.mMipLodBias,
        .anisotropyEnable        = /*_mpDevice->_mpAdapter->requestGPUSettings().mSamplerAnisotropySupported && */ ((desc.mMaxAnisotropy > 0.0f) ? VK_TRUE : VK_FALSE),
        .maxAnisotropy           = desc.mMaxAnisotropy,
        .compareEnable           = to_vk(desc.mCompareFunc) != VK_COMPARE_OP_NEVER ? VK_TRUE : VK_FALSE,
        .compareOp               = to_vk(desc.mCompareFunc),
        .minLod                  = minSamplerLod,
        .maxLod                  = maxSamplerLod,
        .borderColor             = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
        .unnormalizedCoordinates = VK_FALSE

    };
    if (TinyImageFormat_IsPlanar(desc.mSamplerConversionDesc.mFormat))
    {
        auto& conversionDesc = desc.mSamplerConversionDesc;
        auto format          = (VkFormat)TinyImageFormat_ToVkFormat(conversionDesc.mFormat);

        // check format props
        {
            VkFormatProperties formatProps;
            vkGetPhysicalDeviceFormatProperties(_mpDevice->_mpAdapter->handle(), format, &formatProps);
            if (conversionDesc.mChromaOffsetX == SAMPLE_LOCATION_MIDPOINT)
            {
                AXE_ASSERT(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_MIDPOINT_CHROMA_SAMPLES_BIT);
            }
            if (conversionDesc.mChromaOffsetX == SAMPLE_LOCATION_COSITED)
            {
                AXE_ASSERT(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_COSITED_CHROMA_SAMPLES_BIT);
            }
        }

        VkSamplerYcbcrConversionCreateInfo conversionInfo{
            .sType                       = VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_CREATE_INFO,
            .pNext                       = nullptr,
            .format                      = format,
            .ycbcrModel                  = (VkSamplerYcbcrModelConversion)conversionDesc.mModel,
            .ycbcrRange                  = (VkSamplerYcbcrRange)conversionDesc.mRange,
            .components                  = {.r = VK_COMPONENT_SWIZZLE_IDENTITY,
                                            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                                            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                                            .a = VK_COMPONENT_SWIZZLE_IDENTITY},
            .xChromaOffset               = (VkChromaLocation)conversionDesc.mChromaOffsetX,
            .yChromaOffset               = (VkChromaLocation)conversionDesc.mChromaOffsetY,
            .chromaFilter                = to_vk(conversionDesc.mChromaFilter),
            .forceExplicitReconstruction = conversionDesc.mForceExplicitReconstruction ? VK_TRUE : VK_FALSE};
        vkCreateSamplerYcbcrConversion(_mpDevice->_mpHandle, &conversionInfo, nullptr, &_mpVkSamplerYcbcrConversion);
        _mVkSamplerYcbcrConversionInfo = {
            .sType      = VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_INFO,
            .pNext      = nullptr,
            .conversion = _mpVkSamplerYcbcrConversion};
        createInfo.pNext = &_mVkSamplerYcbcrConversionInfo;
    }

    return VK_SUCCEEDED(vkCreateSampler(_mpDevice->_mpHandle, &createInfo, nullptr, &_mpHandle));
}

bool VulkanSampler::_destroy() noexcept
{
    vkDestroySampler(_mpDevice->_mpHandle, _mpHandle, nullptr);
    if (_mpVkSamplerYcbcrConversion != VK_NULL_HANDLE)
    {
        vkDestroySamplerYcbcrConversion(_mpDevice->_mpHandle, _mpVkSamplerYcbcrConversion, nullptr);
    }
    return true;
}

}  // namespace axe::rhi