#pragma once
#include "02Rhi/Rhi.hpp"
#include "VulkanEnums.internal.hpp"

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
    ~VulkanCmd() noexcept override = default;
    void begin() noexcept override;
    void end() noexcept override;
    void setViewport(float x, float y, float width, float height, float minDepth, float maxDepth) noexcept override;
    void setScissor(u32 x, u32 y, u32 width, u32 height) noexcept override;
    void setStencilReferenceValue(u32 val) noexcept override;
    void bindRenderTargets() noexcept override;
    void bindDescriptorSet(u32 index, DescriptorSet* pSet) noexcept override;
    void bindPushConstants() noexcept override;
    void bindPipeline() noexcept override;
    void bindIndexBuffer() noexcept override;
    void bindVertexBuffer() noexcept override;
    void draw(u32 vertexCount, u32 firstIndex) noexcept override;
    void drawInstanced(u32 vertexCount, u32 firstIndex, u32 instanceCount, u32 firstInstance) noexcept override;
    void drawIndexed(u32 indexCount, u32 firstIndex, u32 firstVertex) noexcept override;
    void drawIndexedInstanced(u32 indexCount, u32 firstIndex, u32 instanceCount, u32 firstInstance, u32 firstVertex) noexcept override;
    void dispatch(u32 groupCountX, u32 groupCountY, u32 groupCountZ) noexcept override;
    void resourceBarrier(std::pmr::vector<TextureBarrier>*, std::pmr::vector<BufferBarrier>*, std::pmr::vector<RenderTargetBarrier>*) noexcept override;
    void copyBuffer(Buffer* pDst, Buffer* pSrc, u64 srcOffset, u64 dstOffset, u64 size) noexcept override;
    void updateSubresource(Texture* pDstTexture, Buffer* pSrcBuffer, const SubresourceDataDesc&) noexcept override;
    void updateSubresource(Buffer* pDstBuffer, Texture* pSrcTexture, const SubresourceDataDesc&) noexcept override;
    void resetQueryPool() noexcept override;
    void beginQuery() noexcept override;
    void endQuery() noexcept override;
    void resolveQuery() noexcept override;
    void addDebugMarker() noexcept override;
    void beginDebugMarker() noexcept override;
    void writeDebugMarker() noexcept override;
    void endDebugMarker() noexcept override;

public:
    AXE_PRIVATE auto handle() noexcept { return _mpHandle; }

public:
    AXE_PRIVATE constexpr static auto VK_TYPE_ID = VK_OBJECT_TYPE_COMMAND_BUFFER;

private:
    VkCommandBuffer _mpHandle               = VK_NULL_HANDLE;
    VkRenderPass _mpVkActiveRenderPass      = VK_NULL_HANDLE;
    VkPipelineLayout _mpBoundPipelineLayout = VK_NULL_HANDLE;
    VulkanDevice* const _mpDevice           = nullptr;
    VulkanQueue* _mpQueue                   = nullptr;
    VulkanCmdPool* _mpCmdPool               = nullptr;

    u32 _mCmdBufferCount                    = 0;
    u32 _mType : 3                          = 0;
};

}  // namespace axe::rhi