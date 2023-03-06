#pragma once
#include "02Rhi/Rhi.hpp"
#include "02Rhi/Vulkan/VulkanEnums.hpp"

namespace axe::rhi
{

class VulkanDevice;

class VulkanShader : public Shader
{
private:
    AXE_NON_COPYABLE(VulkanShader);
    VulkanShader(VulkanDevice* device) noexcept : _mpDevice(device) {}
    bool _create(ShaderDesc&) noexcept;
    bool _destroy() noexcept;
    friend class VulkanDevice;

private:
    VulkanDevice* const _mpDevice = nullptr;
    std::array<VkShaderModule, SHADER_STAGE_COUNT> _mpHandles{VK_NULL_HANDLE};
    std::array<std::string_view, SHADER_STAGE_COUNT> _mpEntryNames;
    VkSpecializationInfo* _mpSpecializationInfo = nullptr;
};

}  // namespace axe::rhi