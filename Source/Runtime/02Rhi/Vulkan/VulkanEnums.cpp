#include "02Rhi/Vulkan/VulkanEnums.hpp"

#include <unordered_map>
namespace axe::rhi
{

template <typename T, typename E>
inline static auto to_(const std::pmr::unordered_map<T, E>& table, T t, E e) noexcept
{
    auto iter = table.find(t);
    return iter != table.end() ? iter->second : e;
}

VkFilter to_vk(FilterType type) noexcept
{
    const static std::pmr::unordered_map<FilterType, VkFilter> table{
        {FILTER_TYPE_NEAREST, VK_FILTER_NEAREST},
        {FILTER_TYPE_LINEAR, VK_FILTER_LINEAR},
    };
    return to_(table, type, VK_FILTER_MAX_ENUM);
}

VkSamplerMipmapMode to_vk(MipMapMode type) noexcept
{
    const static std::pmr::unordered_map<MipMapMode, VkSamplerMipmapMode> table{
        {MIPMAP_MODE_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST},
        {MIPMAP_MODE_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR},
    };
    return to_(table, type, VK_SAMPLER_MIPMAP_MODE_MAX_ENUM);
}

VkSamplerAddressMode to_vk(AddressMode type) noexcept
{
    const static std::pmr::unordered_map<AddressMode, VkSamplerAddressMode> table{
        {ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT},
        {ADDRESS_MODE_MIRROR, VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT},
        {ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE},
        {ADDRESS_MODE_CLAMP_TO_BORDER, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER},
    };
    return to_(table, type, VK_SAMPLER_ADDRESS_MODE_MAX_ENUM);
}

VkCompareOp to_vk(CompareMode type) noexcept
{
    const static std::pmr::unordered_map<CompareMode, VkCompareOp> table{
        {CMP_MODE_NEVER, VK_COMPARE_OP_NEVER},
        {CMP_MODE_LESS, VK_COMPARE_OP_LESS},
        {CMP_MODE_EQUAL, VK_COMPARE_OP_EQUAL},
        {CMP_MODE_LEQUAL, VK_COMPARE_OP_LESS_OR_EQUAL},
        {CMP_MODE_GREATER, VK_COMPARE_OP_GREATER},
        {CMP_MODE_NOTEQUAL, VK_COMPARE_OP_NOT_EQUAL},
        {CMP_MODE_GEQUAL, VK_COMPARE_OP_GREATER_OR_EQUAL},
        {CMP_MODE_ALWAYS, VK_COMPARE_OP_ALWAYS},
    };
    return to_(table, type, VK_COMPARE_OP_MAX_ENUM);
}

}  // namespace axe::rhi