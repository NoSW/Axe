#pragma once

#include "02Rhi/Rhi.hpp"
#include "VulkanEnums.internal.hpp"

namespace axe::rhi
{
class VulkanDevice;
class VulkanPipeline : public Pipeline
{
    AXE_NON_COPYABLE(VulkanPipeline);
    VulkanPipeline(VulkanDevice* device) noexcept : _mpDevice(device) {}
    bool _create(const PipelineDesc&) noexcept;
    bool _destroy() noexcept;

    friend class VulkanDevice;

private:
    bool _crateGraphicsPipeline(const PipelineDesc&) noexcept;
    bool _crateComputePipeline(const PipelineDesc&) noexcept;

public:
    ~VulkanPipeline() noexcept override = default;

public:
    auto handle() noexcept { return _mpHandle; }

public:
    constexpr static auto VK_TYPE_ID = VK_OBJECT_TYPE_PIPELINE;

private:
    VulkanDevice* const _mpDevice = nullptr;
    VkPipeline _mpHandle          = VK_NULL_HANDLE;
    PipelineType _mType           = PipelineType::UNDEFINED;
};

}  // namespace axe::rhi