#include "02Rhi/Vulkan/VulkanEnums.hpp"

#include <unordered_map>
namespace axe::rhi
{

// DISCUSSION: switch vs hash_map
// A switch construct is faster (or at least not slower). That's mostly because a switch construct
// gives static data to the compiler, while a runtime structure like a hash map doesn't.

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

VkDescriptorType to_vk_enum(DescriptorType type) noexcept
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

}  // namespace axe::rhi