#pragma once
#include "02Rhi/Rhi.hpp"
#include "02Rhi/Vulkan/VulkanEnums.hpp"

namespace axe::rhi
{

class VulkanDevice;
class VulkanRootSignature;

class VulkanShader final : public Shader
{
private:
    AXE_NON_COPYABLE(VulkanShader);
    VulkanShader(VulkanDevice* device) noexcept : _mpDevice(device) {}
    bool _create(ShaderDesc&) noexcept;
    bool _destroy() noexcept;
    friend class VulkanDevice;
    friend class VulkanRootSignature;

public:
    void* handle() noexcept { return nullptr; }

public:
    ~VulkanShader() noexcept override = default;

public:
    constexpr static VkObjectType TYPE_ID = VK_OBJECT_TYPE_UNKNOWN;

private:
    VulkanDevice* const _mpDevice = nullptr;
    std::array<VkShaderModule, SHADER_STAGE_FLAG_COUNT> _mpHandles{};
    std::array<std::string_view, SHADER_STAGE_FLAG_COUNT> _mpEntryNames;
    VkSpecializationInfo* _mpSpecializationInfo = nullptr;
    ShaderStageFlag _mStage                     = SHADER_STAGE_FLAG_NONE;
    u32 _mNumThreadsPerGroup[3]                 = {0, 0, 0};  // only for computer shader
    PipelineReflection _mReflection{};
};

}  // namespace axe::rhi