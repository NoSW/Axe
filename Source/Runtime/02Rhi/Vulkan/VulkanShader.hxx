#pragma once
#include "02Rhi/Rhi.hpp"
#include "VulkanEnums.hxx"

#include <bit>
#include <list>

namespace axe::rhi
{

class VulkanDevice;
class VulkanRootSignature;
class VulkanPipeline;

class VulkanShader final : public Shader
{
private:
    AXE_NON_COPYABLE(VulkanShader);
    VulkanShader(VulkanDevice* device) noexcept : _mpDevice(device) {}
    bool _create(ShaderDesc&) noexcept;
    bool _destroy() noexcept;
    friend class VulkanDevice;
    friend class VulkanRootSignature;
    friend class VulkanPipeline;

public:
    void* handle() noexcept { return nullptr; }
    ShaderStageFlag stages() const noexcept { return _mReflection.shaderStages; }
    u8 stagesCount() const noexcept { return (u8)std::popcount((u32)_mReflection.shaderStages); }
    bool containStage(ShaderStageFlag stage) const noexcept { return (_mReflection.shaderStages & stage) == stage; }
    std::string_view entryName(ShaderStageFlag stage) const noexcept { return _mpEntryNames[bit2id(stage)]; }
    VkShaderModule handle(ShaderStageFlag stage) noexcept { return _mpHandles[bit2id(stage)]; }
    VkSpecializationInfo* specializationInfo() noexcept { return _mpSpecializationInfo; }
    bool hasTessellation() const noexcept { return containStage(ShaderStageFlag::TESC) && containStage(ShaderStageFlag::TESE); }

public:
    ~VulkanShader() noexcept override = default;

public:
    constexpr static auto VK_TYPE_ID = VK_OBJECT_TYPE_UNKNOWN;

private:
    std::pmr::list<std::pmr::string> _mNamePool;  // all strings reflected from spirv byte code
    VulkanDevice* const _mpDevice = nullptr;
    std::array<VkShaderModule, (u32)ShaderStageFlag::COUNT> _mpHandles{};
    std::array<std::string_view, (u32)ShaderStageFlag::COUNT> _mpEntryNames;
    nullable<VkSpecializationInfo> _mpSpecializationInfo = nullptr;
    u32 _mNumThreadsPerGroup[3]                          = {0, 0, 0};  // only for computer shader
    PipelineReflection _mReflection{};
};

}  // namespace axe::rhi