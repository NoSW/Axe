#include "VulkanEnums.hxx"

#include <tiny_imageformat/tinyimageformat_apis.h>

#include <unordered_map>
namespace axe::rhi
{

// DISCUSSION: switch vs hash_map
// A switch construct is faster (or at least not slower). That's mostly because a switch construct
// gives static data to the compiler, while a runtime structure like a hash map doesn't.

VkFormatFeatureFlags image_usage_to_format_feature(VkImageUsageFlags usage) noexcept
{
    VkFormatFeatureFlags result = (VkFormatFeatureFlags)0;
    if (VK_IMAGE_USAGE_SAMPLED_BIT == (usage & VK_IMAGE_USAGE_SAMPLED_BIT))
    {
        result |= VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
    }
    if (VK_IMAGE_USAGE_STORAGE_BIT == (usage & VK_IMAGE_USAGE_STORAGE_BIT))
    {
        result |= VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT;
    }
    if (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT == (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT))
    {
        result |= VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
    }
    if (VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT == (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT))
    {
        result |= VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }
    return result;
}

VkImageAspectFlags determine_aspect_mask(VkFormat format, bool includeStencilBit) noexcept
{
    VkImageAspectFlags result = 0;
    switch (format)
    {
            // Depth
        case VK_FORMAT_D16_UNORM:
        case VK_FORMAT_X8_D24_UNORM_PACK32:
        case VK_FORMAT_D32_SFLOAT:
            result = VK_IMAGE_ASPECT_DEPTH_BIT;
            break;
            // Stencil
        case VK_FORMAT_S8_UINT:
            result = VK_IMAGE_ASPECT_STENCIL_BIT;
            break;
            // Depth/stencil
        case VK_FORMAT_D16_UNORM_S8_UINT:
        case VK_FORMAT_D24_UNORM_S8_UINT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
            result = VK_IMAGE_ASPECT_DEPTH_BIT;
            if (includeStencilBit) { result |= VK_IMAGE_ASPECT_STENCIL_BIT; }
            break;
            // Assume everything else is Color
        default: result = VK_IMAGE_ASPECT_COLOR_BIT; break;
    }
    return result;
}

VkPipelineStageFlags determine_pipeline_stage_flags(DeterminePipelineStageOption option) noexcept
{
    VkPipelineStageFlags flags = 0;
    switch (option.queueType)
    {
        case QUEUE_TYPE_FLAG_GRAPHICS:
        {
            if ((option.mAccessFlags & (VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT)))
            {
                flags |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
            }

            if ((option.mAccessFlags & (VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT)))
            {
                flags |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
                flags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                if (option.supportGeomShader)
                {
                    flags |= VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
                }
                if (option.supportTeseShader)
                {
                    flags |= VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT;
                    flags |= VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
                }
                flags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
                if (option.supportRayTracing)
                {
                    flags |= VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
                }
            }

            if ((option.mAccessFlags & VK_ACCESS_INPUT_ATTACHMENT_READ_BIT))
            {
                flags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            }

            if ((option.mAccessFlags & (VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)))
            {
                flags |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            }

            if ((option.mAccessFlags & (VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)))
                flags |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            break;
        }
        case QUEUE_TYPE_FLAG_COMPUTE:
        {
            if ((option.mAccessFlags & (VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT)) ||
                (option.mAccessFlags & VK_ACCESS_INPUT_ATTACHMENT_READ_BIT) ||
                (option.mAccessFlags & (VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)) ||
                (option.mAccessFlags & (VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)))
            {
                return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
            }
            if ((option.mAccessFlags & (VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT)))
            {
                flags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            }
            break;
        }
        case QUEUE_TYPE_FLAG_TRANSFER: return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        default: break;
    }

    // Compatible with both compute and graphics queues
    if ((option.mAccessFlags & VK_ACCESS_INDIRECT_COMMAND_READ_BIT))
    {
        flags |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
    }
    if ((option.mAccessFlags & (VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT)))
    {
        flags |= VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    if ((option.mAccessFlags & (VK_ACCESS_HOST_READ_BIT | VK_ACCESS_HOST_WRITE_BIT)))
    {
        flags |= VK_PIPELINE_STAGE_HOST_BIT;
    }
    if (flags == 0)
    {
        flags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    }

    return flags;
}

VkAccessFlags resource_state_to_access_flags(ResourceStateFlags state) noexcept
{
    VkAccessFlags ret = 0;
    if (state & RESOURCE_STATE_COPY_SOURCE) { ret |= VK_ACCESS_TRANSFER_READ_BIT; }
    if (state & RESOURCE_STATE_COPY_DEST) { ret |= VK_ACCESS_TRANSFER_WRITE_BIT; }
    if (state & RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER) { ret |= VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT; }
    if (state & RESOURCE_STATE_INDEX_BUFFER) { ret |= VK_ACCESS_INDEX_READ_BIT; }
    if (state & RESOURCE_STATE_UNORDERED_ACCESS) { ret |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT; }
    if (state & RESOURCE_STATE_INDIRECT_ARGUMENT) { ret |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT; }
    if (state & RESOURCE_STATE_RENDER_TARGET) { ret |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; }
    if (state & RESOURCE_STATE_DEPTH_WRITE) { ret |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT; }
    if (state & RESOURCE_STATE_SHADER_RESOURCE) { ret |= VK_ACCESS_SHADER_READ_BIT; }
    if (state & RESOURCE_STATE_PRESENT) { ret |= VK_ACCESS_MEMORY_READ_BIT; }
    if (state & RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) { ret |= VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR; }
    return ret;
}

VkImageLayout resource_state_to_image_layout(ResourceStateFlags usage) noexcept
{
    if (usage & RESOURCE_STATE_COPY_SOURCE) { return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL; }
    else if (usage & RESOURCE_STATE_COPY_DEST) { return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL; }
    else if (usage & RESOURCE_STATE_RENDER_TARGET) { return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; }
    else if (usage & RESOURCE_STATE_DEPTH_WRITE) { return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; }
    else if (usage & RESOURCE_STATE_UNORDERED_ACCESS) { return VK_IMAGE_LAYOUT_GENERAL; }
    else if (usage & RESOURCE_STATE_SHADER_RESOURCE) { return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; }
    else if (usage & RESOURCE_STATE_PRESENT) { return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; }
    else if (usage == RESOURCE_STATE_COMMON) { return VK_IMAGE_LAYOUT_GENERAL; }
    else { return VK_IMAGE_LAYOUT_UNDEFINED; }
}

VkBufferUsageFlags to_buffer_usage(DescriptorTypeFlag usage, bool texel) noexcept
{
    VkBufferUsageFlags result = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    if (usage & DESCRIPTOR_TYPE_UNIFORM_BUFFER)
    {
        result |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    }
    if (usage & DESCRIPTOR_TYPE_RW_BUFFER)
    {
        result |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        if (texel) { result |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT; }
    }
    if (usage & DESCRIPTOR_TYPE_BUFFER)
    {
        result |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        if (texel) { result |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT; }
    }
    if (usage & DESCRIPTOR_TYPE_INDEX_BUFFER)
    {
        result |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    }
    if (usage & DESCRIPTOR_TYPE_VERTEX_BUFFER)
    {
        result |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    }
    if (usage & DESCRIPTOR_TYPE_INDIRECT_BUFFER)
    {
        result |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    }
    if (usage & DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_VKONLY)
    {
        result |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR;
    }
    if (usage & DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_BUILD_INPUT_VKONLY)
    {
        result |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
    }
    if (usage & DESCRIPTOR_TYPE_SHADER_DEVICE_ADDRESS_VKONLY)
    {
        result |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    }
    if (usage & DESCRIPTOR_TYPE_SHADER_BINDING_TABLE_VKONLY)
    {
        result |= VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR;
    }
    return result;
}

VkImageUsageFlags to_image_usage(DescriptorTypeFlag usage) noexcept
{
    {
        VkImageUsageFlags result = 0;
        if (DESCRIPTOR_TYPE_TEXTURE == (usage & DESCRIPTOR_TYPE_TEXTURE))
        {
            result |= VK_IMAGE_USAGE_SAMPLED_BIT;
        }

        if (DESCRIPTOR_TYPE_RW_TEXTURE == (usage & DESCRIPTOR_TYPE_RW_TEXTURE))
        {
            result |= VK_IMAGE_USAGE_STORAGE_BIT;
        }
        return result;
    }
}

VkPipelineBindPoint to_pipeline_bind_point(PipelineType type) noexcept
{
    switch (type)
    {
        case PIPELINE_TYPE_COMPUTE: return VK_PIPELINE_BIND_POINT_COMPUTE;
        case PIPELINE_TYPE_GRAPHICS: return VK_PIPELINE_BIND_POINT_GRAPHICS;
        case PIPELINE_TYPE_RAYTRACING: return VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;
        default: AXE_ASSERT(false, "Invalid Type"); return VK_PIPELINE_BIND_POINT_MAX_ENUM;
    }
}

VkFilter to_vk_enum(FilterType type) noexcept
{
    switch (type)
    {
        case FILTER_TYPE_NEAREST: return VK_FILTER_NEAREST;
        case FILTER_TYPE_LINEAR: return VK_FILTER_LINEAR;
        default: AXE_ASSERT(false, "Invalid Type"); return VK_FILTER_MAX_ENUM;
    }
}

VkSamplerMipmapMode to_vk_enum(MipMapMode type) noexcept
{
    switch (type)
    {
        case MIPMAP_MODE_NEAREST: return VK_SAMPLER_MIPMAP_MODE_NEAREST;
        case MIPMAP_MODE_LINEAR: return VK_SAMPLER_MIPMAP_MODE_LINEAR;
        default: AXE_ASSERT(false, "Invalid Type"); return VK_SAMPLER_MIPMAP_MODE_MAX_ENUM;
    }
}

VkSamplerAddressMode to_vk_enum(AddressMode type) noexcept
{
    switch (type)
    {
        case ADDRESS_MODE_REPEAT: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case ADDRESS_MODE_MIRROR: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        case ADDRESS_MODE_CLAMP_TO_EDGE: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case ADDRESS_MODE_CLAMP_TO_BORDER: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        default: AXE_ASSERT(false, "Invalid Type"); return VK_SAMPLER_ADDRESS_MODE_MAX_ENUM;
    }
}

VkCompareOp to_vk_enum(CompareMode type) noexcept
{
    switch (type)
    {
        case CMP_MODE_NEVER: return VK_COMPARE_OP_NEVER;
        case CMP_MODE_LESS: return VK_COMPARE_OP_LESS;
        case CMP_MODE_EQUAL: return VK_COMPARE_OP_EQUAL;
        case CMP_MODE_LEQUAL: return VK_COMPARE_OP_LESS_OR_EQUAL;
        case CMP_MODE_GREATER: return VK_COMPARE_OP_GREATER;
        case CMP_MODE_NOTEQUAL: return VK_COMPARE_OP_NOT_EQUAL;
        case CMP_MODE_GEQUAL: return VK_COMPARE_OP_GREATER_OR_EQUAL;
        case CMP_MODE_ALWAYS: return VK_COMPARE_OP_ALWAYS;
        default: AXE_ASSERT(false, "Invalid Type"); return VK_COMPARE_OP_MAX_ENUM;
    }
}

VkDescriptorType to_vk_enum(DescriptorTypeFlag type) noexcept
{
    switch (type)
    {
        case DESCRIPTOR_TYPE_SAMPLER: return VK_DESCRIPTOR_TYPE_SAMPLER;
        case DESCRIPTOR_TYPE_TEXTURE: return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        case DESCRIPTOR_TYPE_UNIFORM_BUFFER: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case DESCRIPTOR_TYPE_RW_TEXTURE: return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        case DESCRIPTOR_TYPE_BUFFER:
        case DESCRIPTOR_TYPE_RW_BUFFER: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        case DESCRIPTOR_TYPE_INPUT_ATTACHMENT_VKONLY: return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        case DESCRIPTOR_TYPE_TEXEL_BUFFER_VKONLY: return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        case DESCRIPTOR_TYPE_RW_TEXEL_BUFFER_VKONLY: return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
        case DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER_VKONLY: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        case DESCRIPTOR_TYPE_RAY_TRACING: return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        default: AXE_ASSERT(false, "Invalid Type"); return VK_DESCRIPTOR_TYPE_MAX_ENUM;
    }
}

VkShaderStageFlags to_vk_enum(ShaderStageFlag stages) noexcept
{
    VkShaderStageFlags res = 0;
    if (stages & SHADER_STAGE_FLAG_GRAPHICS)
        return VK_SHADER_STAGE_ALL_GRAPHICS;

    if (stages & SHADER_STAGE_FLAG_VERT)
        res |= VK_SHADER_STAGE_VERTEX_BIT;
    if (stages & SHADER_STAGE_FLAG_GEOM)
        res |= VK_SHADER_STAGE_GEOMETRY_BIT;
    if (stages & SHADER_STAGE_FLAG_TESE)
        res |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    if (stages & SHADER_STAGE_FLAG_TESC)
        res |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    if (stages & SHADER_STAGE_FLAG_COMP)
        res |= VK_SHADER_STAGE_COMPUTE_BIT;
    if (stages & SHADER_STAGE_FLAG_RAYTRACING)
        res |=
            (VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
             VK_SHADER_STAGE_MISS_BIT_KHR | VK_SHADER_STAGE_INTERSECTION_BIT_KHR | VK_SHADER_STAGE_CALLABLE_BIT_KHR);

    AXE_ASSERT(res != 0, "Invalid Type");
    return res;
}

VkSampleCountFlagBits to_vk_enum(MSAASampleCount type) noexcept
{
    switch (type)
    {
        case MSAA_SAMPLE_COUNT_1: return VK_SAMPLE_COUNT_1_BIT;
        case MSAA_SAMPLE_COUNT_2: return VK_SAMPLE_COUNT_2_BIT;
        case MSAA_SAMPLE_COUNT_4: return VK_SAMPLE_COUNT_4_BIT;
        case MSAA_SAMPLE_COUNT_8: return VK_SAMPLE_COUNT_8_BIT;
        case MSAA_SAMPLE_COUNT_16: return VK_SAMPLE_COUNT_16_BIT;
        default: AXE_ASSERT(false, "Invalid Type"); return VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM;
    }
}

VkFormat to_vk_enum(TinyImageFormat fmt) noexcept { return (VkFormat)TinyImageFormat_ToVkFormat(fmt); }

// clang-format off
VkBlendFactor to_vk_enum(BlendConstant bc) noexcept
{
    switch (bc)
    {
        case BC_ZERO:                   return VK_BLEND_FACTOR_ZERO;
        case BC_ONE:                    return VK_BLEND_FACTOR_ONE;
        case BC_SRC_COLOR:              return VK_BLEND_FACTOR_SRC_COLOR;
        case BC_ONE_MINUS_SRC_COLOR:    return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        case BC_DST_COLOR:              return VK_BLEND_FACTOR_DST_COLOR;
        case BC_ONE_MINUS_DST_COLOR:    return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
        case BC_SRC_ALPHA:              return VK_BLEND_FACTOR_SRC_ALPHA;
        case BC_ONE_MINUS_SRC_ALPHA:    return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        case BC_DST_ALPHA:              return VK_BLEND_FACTOR_DST_ALPHA;
        case BC_ONE_MINUS_DST_ALPHA:    return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        case BC_SRC_ALPHA_SATURATE:     return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
        case BC_BLEND_FACTOR:           return VK_BLEND_FACTOR_CONSTANT_COLOR;
        case BC_ONE_MINUS_BLEND_FACTOR: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
        default: AXE_ASSERT(false, "Invalid Type"); return VK_BLEND_FACTOR_MAX_ENUM;
    }
}


VkBlendOp to_vk_enum(BlendMode bm) noexcept
{
    switch (bm)
    {
        case BM_ADD:              return VK_BLEND_OP_ADD;
        case BM_SUBTRACT:         return VK_BLEND_OP_SUBTRACT;
        case BM_REVERSE_SUBTRACT: return VK_BLEND_OP_REVERSE_SUBTRACT;
        case BM_MIN:              return VK_BLEND_OP_MIN;
        case BM_MAX:              return VK_BLEND_OP_MAX;
        default: AXE_ASSERT(false, "Invalid Type"); return VK_BLEND_OP_MAX_ENUM;
    }
}

// clang-format on

VkPipelineColorBlendStateCreateInfo to_vk_struct(const BlendStateDesc& blendDesc) noexcept
{
    std::array<VkPipelineColorBlendAttachmentState, MAX_RENDER_TARGET_ATTACHMENTS> vkBlendStates;
    for (u32 i = 0; i < blendDesc.attachmentCount; ++i)
    {
        if (!blendDesc.isIndependentBlend && i > 0) { break; }

        if (blendDesc.renderTargetMask & (1 << i))
        {
            const auto& rt = blendDesc.perRenderTarget[i];

            VkBool32 blendDisable =
                (to_vk_enum(rt.srcFactor) == VK_BLEND_FACTOR_ONE &&
                 to_vk_enum(rt.dstFactor) == VK_BLEND_FACTOR_ZERO &&
                 to_vk_enum(rt.srcAlphaFactor) == VK_BLEND_FACTOR_ONE &&
                 to_vk_enum(rt.dstAlphaFactor) == VK_BLEND_FACTOR_ZERO);

            vkBlendStates[i].blendEnable         = !blendDisable;
            vkBlendStates[i].colorWriteMask      = rt.mask;
            vkBlendStates[i].srcColorBlendFactor = to_vk_enum(rt.srcFactor);
            vkBlendStates[i].dstColorBlendFactor = to_vk_enum(rt.dstFactor);
            vkBlendStates[i].srcAlphaBlendFactor = to_vk_enum(rt.srcAlphaFactor);
            vkBlendStates[i].dstAlphaBlendFactor = to_vk_enum(rt.dstAlphaFactor);
            vkBlendStates[i].colorBlendOp        = to_vk_enum(rt.blendMode);
            vkBlendStates[i].alphaBlendOp        = to_vk_enum(rt.blendAlphaMode);
        }
    }

    return VkPipelineColorBlendStateCreateInfo{
        .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext           = nullptr,
        .flags           = 0,
        .logicOpEnable   = VK_FALSE,
        .logicOp         = VK_LOGIC_OP_CLEAR,
        .attachmentCount = blendDesc.attachmentCount,
        .pAttachments    = vkBlendStates.data(),
        .blendConstants{0.0f, 0.0f, 0.0f, 0.0f}};
}

}  // namespace axe::rhi