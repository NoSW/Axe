#pragma once

#include "02Rhi/RhiEnums.hpp"
#include "02Rhi/RhiStructs.hpp"

#define VK_NO_PROTOTYPES  // used for volk
#include <vulkan/vulkan.h>

#include <vk_enum_string_helper.h>

namespace axe::rhi
{

enum
{
    MAX_DESCRIPTOR_POOL_SIZE_ARRAY_COUNT = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT + 1,  // maximum count of descriptor pool for each set
    MAX_PLANE_COUNT                      = 3,
    VK_MAX_ATTACHMENT_ARRAY_COUNT        = ((MAX_RENDER_TARGET_ATTACHMENTS + 2) * 2)
};

// Use vkSetDeviceMemoryPriorityEXT to provide the OS with information on which allocations should stay in memory and which should be demoted first when video memory is limited.
// The highest priority should be given to GPU-written resources like color attachments, depth attachments, storage images, and buffers written from the GPU.

inline constexpr float VK_DEVICE_MEM_PRIORITY_HIGHEST = 1.0f;
inline constexpr float VK_DEVICE_MEM_PRIORITY_MIDDLE  = 0.5f;
inline constexpr float VK_DEVICE_MEM_PRIORITY_LOWEST  = 0.0f;

struct DeterminePipelineStageOption
{
    VkAccessFlags mAccessFlags;
    QueueTypeFlag queueType : 5 = QueueTypeFlag::UNDEFINED;
    u8 supportGeomShader    : 1 = 0;
    u8 supportTeseShader    : 1 = 0;
    u8 supportRayTracing    : 1 = 0;
};
// clang-format off

// enum to (another) enum
VkFormatFeatureFlags    image_usage_to_format_feature(VkImageUsageFlags) noexcept;
VkImageAspectFlags      determine_aspect_mask(VkFormat format, bool includeStencilBit) noexcept;
VkPipelineStageFlags    determine_pipeline_stage_flags(DeterminePipelineStageOption) noexcept;
VkAccessFlags           resource_state_to_access_flags(ResourceStateFlags state) noexcept;
VkImageLayout           resource_state_to_image_layout(ResourceStateFlags usage) noexcept;
VkBufferUsageFlags      to_buffer_usage(DescriptorTypeFlag usage, bool texel) noexcept;
VkImageUsageFlags       to_image_usage(DescriptorTypeFlag usage) noexcept;
VkPipelineBindPoint     to_pipeline_bind_point(PipelineType type) noexcept;

// enum to enum
VkFilter                to_vk_enum(FilterType) noexcept;
VkSamplerMipmapMode     to_vk_enum(MipMapMode) noexcept;
VkSamplerAddressMode    to_vk_enum(AddressMode) noexcept;
VkCompareOp             to_vk_enum(CompareMode) noexcept;
VkDescriptorType        to_vk_enum(DescriptorTypeFlag) noexcept;
VkShaderStageFlags      to_vk_enum(ShaderStageFlag) noexcept;
VkSampleCountFlagBits   to_vk_enum(MSAASampleCount) noexcept;
VkFormat                to_vk_enum(TinyImageFormat) noexcept;
VkBlendFactor           to_vk_enum(BlendConstant) noexcept;
VkBlendOp               to_vk_enum(BlendMode) noexcept;
VkAttachmentLoadOp      to_vk_enum(LoadActionType) noexcept;
VkAttachmentStoreOp     to_vk_enum(StoreActionType) noexcept;
VkVertexInputRate       to_vk_enum(VertexAttribRate) noexcept;
VkPrimitiveTopology     to_vk_enum(PrimitiveTopology) noexcept;
VkPolygonMode           to_vk_enum(FillMode) noexcept;
VkCullModeFlags         to_vk_enum(CullMode) noexcept;
VkFrontFace             to_vk_enum(FrontFace) noexcept;
VkStencilOp             to_vk_enum(StencilOp) noexcept;
VkColorComponentFlags   to_vk_enum(Channel) noexcept;

// struct to struct
VkPipelineColorBlendStateCreateInfo to_vk_struct(const BlendStateDesc& blendDesc) noexcept; // TODO: remove this one-use  function
// clang-format on

}  // namespace axe::rhi