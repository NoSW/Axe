#pragma once

#include "02Rhi/RhiEnums.hpp"
#include "02Rhi/RhiStructs.hpp"

#define VK_NO_PROTOTYPES  // used for volk
#include <vulkan/vulkan.h>

#include <vulkan/vk_enum_string_helper.h>

namespace axe::rhi
{

enum
{
    MAX_DESCRIPTOR_POOL_SIZE_ARRAY_COUNT = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT + 1,
    MAX_PLANE_COUNT                      = 3
};

struct DeterminePipelineStageOption
{
    VkAccessFlags mAccessFlags;
    QueueType mQueueType    : 5 = QUEUE_TYPE_MAX;
    u8 mIsSupportGeomShader : 1 = 0;
    u8 mIsSupportTeseShader : 1 = 0;
    u8 mIsSupportRayTracing : 1 = 0;
};
// clang-format off

// enum to (another) enum
VkFormatFeatureFlags    image_usage_to_format_feature(VkImageUsageFlags) noexcept;
VkImageAspectFlags      determine_aspect_mask(VkFormat format, bool includeStencilBit) noexcept;
VkPipelineStageFlags    determine_pipeline_stage_flags(DeterminePipelineStageOption) noexcept;
VkAccessFlags           resource_state_to_access_flags(ResourceState state) noexcept;
VkImageLayout           resource_state_to_image_layout(ResourceState usage) noexcept;
VkBufferUsageFlags      to_buffer_usage(DescriptorType usage, bool texel) noexcept;
VkImageUsageFlags       to_image_usage(DescriptorType usage) noexcept;

// enum to enum
VkFilter                to_vk_enum(FilterType) noexcept;
VkSamplerMipmapMode     to_vk_enum(MipMapMode) noexcept;
VkSamplerAddressMode    to_vk_enum(AddressMode) noexcept;
VkCompareOp             to_vk_enum(CompareMode) noexcept;
VkDescriptorType        to_vk_enum(DescriptorType) noexcept;
VkShaderStageFlags      to_vk_enum(ShaderStageFlag) noexcept;
VkSampleCountFlagBits   to_vk_enum(MSAASampleCount) noexcept;
VkFormat                to_vk_enum(TinyImageFormat) noexcept;
VkBlendFactor           to_vk_enum(BlendConstant) noexcept;
VkBlendOp               to_vk_enum(BlendMode) noexcept;

// struct to struct
VkPipelineColorBlendStateCreateInfo to_vk_struct(const BlendStateDesc& blendDesc) noexcept; // TODO: remove this one-use  function
// clang-format on

}  // namespace axe::rhi