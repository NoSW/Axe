#pragma once
#include "02Rhi/Rhi.hpp"
#include "VulkanEnums.hxx"

namespace axe::rhi
{
class VulkanDevice;
class VulkanQueue;
class VulkanCmdPool;

class VulkanCmd final : public Cmd
{
    friend class VulkanDevice;
    friend class VulkanQueue;
    AXE_NON_COPYABLE(VulkanCmd);
    VulkanCmd(VulkanDevice* device) noexcept : _mpDevice(device) {}
    bool _create(CmdDesc&) noexcept;
    bool _destroy() noexcept;

public:
    AXE_PUBLIC ~VulkanCmd() noexcept override = default;
    AXE_PUBLIC void begin() noexcept override;
    AXE_PUBLIC void end() noexcept override;
    AXE_PUBLIC void setViewport(float x, float y, float width, float height, float minDepth, float maxDepth) noexcept override;
    AXE_PUBLIC void setScissor(u32 x, u32 y, u32 width, u32 height) noexcept override;
    AXE_PUBLIC void setStencilReferenceValue(u32 val) noexcept override;
    AXE_PUBLIC void bindRenderTargets() noexcept override;
    AXE_PUBLIC void bindDescriptorSet(u32 index, DescriptorSet* pSet) noexcept override;
    AXE_PUBLIC void bindPushConstants() noexcept override;
    AXE_PUBLIC void bindPipeline() noexcept override;
    AXE_PUBLIC void bindIndexBuffer() noexcept override;
    AXE_PUBLIC void bindVertexBuffer() noexcept override;
    AXE_PUBLIC void draw(u32 vertexCount, u32 firstIndex) noexcept override;
    AXE_PUBLIC void drawInstanced(u32 vertexCount, u32 firstIndex, u32 instanceCount, u32 firstInstance) noexcept override;
    AXE_PUBLIC void drawIndexed(u32 indexCount, u32 firstIndex, u32 firstVertex) noexcept override;
    AXE_PUBLIC void drawIndexedInstanced(u32 indexCount, u32 firstIndex, u32 instanceCount, u32 firstInstance, u32 firstVertex) noexcept override;
    AXE_PUBLIC void dispatch(u32 groupCountX, u32 groupCountY, u32 groupCountZ) noexcept override;
    AXE_PUBLIC void resourceBarrier(std::pmr::vector<TextureBarrier>*, std::pmr::vector<BufferBarrier>*, std::pmr::vector<RenderTargetBarrier>*) noexcept override;
    AXE_PUBLIC void updateBuffer() noexcept override;
    AXE_PUBLIC void updateSubresource() noexcept override;
    AXE_PUBLIC void copySubresource() noexcept override;
    AXE_PUBLIC void resetQueryPool() noexcept override;
    AXE_PUBLIC void beginQuery() noexcept override;
    AXE_PUBLIC void endQuery() noexcept override;
    AXE_PUBLIC void resolveQuery() noexcept override;
    AXE_PUBLIC void addDebugMarker() noexcept override;
    AXE_PUBLIC void beginDebugMarker() noexcept override;
    AXE_PUBLIC void writeDebugMarker() noexcept override;
    AXE_PUBLIC void endDebugMarker() noexcept override;

public:
    auto handle() noexcept { return _mpHandle; }

public:
    constexpr static VkObjectType getVkTypeId() noexcept { return VK_OBJECT_TYPE_COMMAND_BUFFER; }

private:
    VkCommandBuffer _mpHandle               = VK_NULL_HANDLE;
    VkRenderPass _mpVkActiveRenderPass      = VK_NULL_HANDLE;
    VkPipelineLayout _mpBoundPipelineLayout = VK_NULL_HANDLE;
    VulkanQueue* _mpQueue                   = nullptr;
    VulkanDevice* const _mpDevice           = nullptr;
    VulkanCmdPool* _mpCmdPool               = nullptr;

    u32 _mCmdBufferCount                    = 0;
    u32 _mType : 3                          = 0;
};

}  // namespace axe::rhi