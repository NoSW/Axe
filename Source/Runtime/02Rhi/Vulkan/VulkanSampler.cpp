#include "02Rhi/Vulkan/VulkanSampler.hpp"
#include "02Rhi/Vulkan/VulkanDevice.hpp"
#include "02Rhi/Vulkan/VulkanEnums.hpp"
#include <volk/volk.h>

#include <tiny_imageformat/tinyimageformat_apis.h>
#include <tiny_imageformat/tinyimageformat_query.h>

namespace axe::rhi
{

bool VulkanSampler::_create(const SamplerDesc& desc) noexcept
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
        .magFilter               = to_vk_enum(desc.mMagFilter),
        .minFilter               = to_vk_enum(desc.mMinFilter),
        .mipmapMode              = to_vk_enum(desc.mMipMapMode),
        .addressModeU            = to_vk_enum(desc.mAddressU),
        .addressModeV            = to_vk_enum(desc.mAddressV),
        .addressModeW            = to_vk_enum(desc.mAddressW),
        .mipLodBias              = desc.mMipLodBias,
        .anisotropyEnable        = /*_mpDevice->_mpAdapter->requestGPUSettings().mSamplerAnisotropySupported && */ ((desc.mMaxAnisotropy > 0.0f) ? VK_TRUE : VK_FALSE),
        .maxAnisotropy           = desc.mMaxAnisotropy,
        .compareEnable           = to_vk_enum(desc.mCompareFunc) != VK_COMPARE_OP_NEVER ? VK_TRUE : VK_FALSE,
        .compareOp               = to_vk_enum(desc.mCompareFunc),
        .minLod                  = minSamplerLod,
        .maxLod                  = maxSamplerLod,
        .borderColor             = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
        .unnormalizedCoordinates = VK_FALSE

    };
    if (TinyImageFormat_IsPlanar(desc.mSamplerConversionDesc.mFormat))
    {
        auto& conversionDesc = desc.mSamplerConversionDesc;
        auto format          = to_vk_enum(conversionDesc.mFormat);

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
            .chromaFilter                = to_vk_enum(conversionDesc.mChromaFilter),
            .forceExplicitReconstruction = conversionDesc.mForceExplicitReconstruction ? VK_TRUE : VK_FALSE};
        vkCreateSamplerYcbcrConversion(_mpDevice->handle(), &conversionInfo, nullptr, &_mpVkSamplerYcbcrConversion);
        _mVkSamplerYcbcrConversionInfo = {
            .sType      = VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_INFO,
            .pNext      = nullptr,
            .conversion = _mpVkSamplerYcbcrConversion};
        createInfo.pNext = &_mVkSamplerYcbcrConversionInfo;
    }

    return VK_SUCCEEDED(vkCreateSampler(_mpDevice->handle(), &createInfo, nullptr, &_mpHandle));
}

bool VulkanSampler::_destroy() noexcept
{
    vkDestroySampler(_mpDevice->handle(), _mpHandle, nullptr);
    _mpHandle = VK_NULL_HANDLE;

    if (_mpVkSamplerYcbcrConversion != VK_NULL_HANDLE)
    {
        vkDestroySamplerYcbcrConversion(_mpDevice->handle(), _mpVkSamplerYcbcrConversion, nullptr);
    }
    return true;
}

}  // namespace axe::rhi