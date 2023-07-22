#include "VulkanSampler.internal.hpp"
#include "VulkanDevice.internal.hpp"
#include "VulkanEnums.internal.hpp"
#include <volk.h>

#include <tiny_imageformat/tinyimageformat_apis.h>
#include <tiny_imageformat/tinyimageformat_query.h>

namespace axe::rhi
{

bool VulkanSampler::_create(const SamplerDesc& desc) noexcept
{
    // default lod values
    float minSamplerLod = 0;
    float maxSamplerLod = desc.mipMapMode == MipMapMode::LINEAR ? VK_LOD_CLAMP_NONE : 0;

    // user provided lod values
    if (desc.isSetLodRange)
    {
        minSamplerLod = desc.minLod;
        maxSamplerLod = desc.maxLod;
    }

    VkSamplerCreateInfo createInfo{
        .sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext                   = nullptr,
        .flags                   = 0,
        .magFilter               = to_vk_enum(desc.magFilter),
        .minFilter               = to_vk_enum(desc.minFilter),
        .mipmapMode              = to_vk_enum(desc.mipMapMode),
        .addressModeU            = to_vk_enum(desc.addressU),
        .addressModeV            = to_vk_enum(desc.addressV),
        .addressModeW            = to_vk_enum(desc.addressW),
        .mipLodBias              = desc.mipLodBias,
        .anisotropyEnable        = /*_mpDevice->_mpAdapter->requestGPUSettings().mSamplerAnisotropySupported && */ ((desc.maxAnisotropy > 0.0f) ? VK_TRUE : VK_FALSE),
        .maxAnisotropy           = desc.maxAnisotropy,
        .compareEnable           = to_vk_enum(desc.compareFunc) != VK_COMPARE_OP_NEVER ? VK_TRUE : VK_FALSE,
        .compareOp               = to_vk_enum(desc.compareFunc),
        .minLod                  = minSamplerLod,
        .maxLod                  = maxSamplerLod,
        .borderColor             = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
        .unnormalizedCoordinates = VK_FALSE

    };
    if (TinyImageFormat_IsPlanar(desc.samplerConversionDesc.format))
    {
        auto& conversionDesc = desc.samplerConversionDesc;
        auto format          = to_vk_enum(conversionDesc.format);

        // check format props
        {
            VkFormatProperties formatProps;
            vkGetPhysicalDeviceFormatProperties(_mpDevice->_mpAdapter->handle(), format, &formatProps);
            if (conversionDesc.chromaOffsetX == SampleLocation::MIDPOINT)
            {
                AXE_ASSERT(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_MIDPOINT_CHROMA_SAMPLES_BIT);
            }
            if (conversionDesc.chromaOffsetX == SampleLocation::COSITED)
            {
                AXE_ASSERT(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_COSITED_CHROMA_SAMPLES_BIT);
            }
        }

        VkSamplerYcbcrConversionCreateInfo conversionInfo{
            .sType                       = VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_CREATE_INFO,
            .pNext                       = nullptr,
            .format                      = format,
            .ycbcrModel                  = (VkSamplerYcbcrModelConversion)conversionDesc.model,
            .ycbcrRange                  = (VkSamplerYcbcrRange)conversionDesc.range,
            .components                  = {.r = VK_COMPONENT_SWIZZLE_IDENTITY,
                                            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                                            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                                            .a = VK_COMPONENT_SWIZZLE_IDENTITY},
            .xChromaOffset               = (VkChromaLocation)conversionDesc.chromaOffsetX,
            .yChromaOffset               = (VkChromaLocation)conversionDesc.chromaOffsetY,
            .chromaFilter                = to_vk_enum(conversionDesc.chromaFilter),
            .forceExplicitReconstruction = conversionDesc.isForceExplicitReconstruction ? VK_TRUE : VK_FALSE};
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