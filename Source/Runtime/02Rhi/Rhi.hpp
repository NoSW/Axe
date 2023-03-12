/* All public interfaces of 02Rhi */
#pragma once

#include "02Rhi/RhiStructs.hpp"

namespace axe::rhi
{

class RhiObjectBase
{
#ifdef _DEBUG
public:
    std::string_view getDebugLabel() const noexcept { return _mLabel; }
    void setDebugLabel(std::string_view label) noexcept { _mLabel = label; }

private:
    // developer-provided label which is used in an implementation-defined way.
    // It can be used by high layers, or other tools to help identify the underlying internal object to the developer.
    // Examples include displaying the label in GPUError messages, console warnings, and platform debugging utilities.
    std::pmr::string _mLabel = "Untitled";
#else
    constexpr std::string_view getDebugLabel() const noexcept
    {
        return "Untitled";
    }
    constexpr void setDebugLabel(std::string_view label) noexcept {}
#endif
};

///////////////////////////////////////////////
//                    Backend
///////////////////////////////////////////////
class Backend : public RhiObjectBase
{
public:
    virtual ~Backend() noexcept                            = default;
    virtual Adapter* requestAdapter(AdapterDesc&) noexcept = 0;
    virtual void releaseAdapter(Adapter*&) noexcept        = 0;
};

AXE_PUBLIC Backend* createBackend(GraphicsApi, BackendDesc&) noexcept;
AXE_PUBLIC void destroyBackend(Backend*&) noexcept;

///////////////////////////////////////////////
//                    Adapter
///////////////////////////////////////////////

inline constexpr u8 MAX_NUM_ADAPTER_PER_BACKEND = 4;
inline constexpr u8 MAX_NUM_DEVICE_PER_ADAPTER  = 4;

class Adapter : public RhiObjectBase
{
public:
    virtual ~Adapter() noexcept                         = default;
    virtual Device* requestDevice(DeviceDesc&) noexcept = 0;
    virtual void releaseDevice(Device*&) noexcept       = 0;
    virtual GPUSettings requestGPUSettings() noexcept   = 0;
};

///////////////////////////////////////////////
//                    Device
///////////////////////////////////////////////

class Device : public RhiObjectBase
{
public:
    virtual ~Device() noexcept                                                         = default;
    [[nodiscard]] virtual Semaphore* createSemaphore(SemaphoreDesc&) noexcept          = 0;
    [[nodiscard]] virtual Fence* createFence(FenceDesc&) noexcept                      = 0;
    [[nodiscard]] virtual Queue* requestQueue(QueueDesc&) noexcept                     = 0;
    [[nodiscard]] virtual SwapChain* createSwapChain(SwapChainDesc&) noexcept          = 0;
    [[nodiscard]] virtual CmdPool* createCmdPool(CmdPoolDesc&) noexcept                = 0;
    [[nodiscard]] virtual Cmd* createCmd(CmdDesc&) noexcept                            = 0;
    [[nodiscard]] virtual Sampler* createSampler(SamplerDesc&) noexcept                = 0;
    [[nodiscard]] virtual RenderTarget* createRenderTarget(RenderTargetDesc&) noexcept = 0;
    [[nodiscard]] virtual Shader* createShader(ShaderDesc&) noexcept                   = 0;
    virtual bool destroySemaphore(Semaphore*&) noexcept                                = 0;
    virtual bool destroyFence(Fence*&) noexcept                                        = 0;
    virtual bool releaseQueue(Queue*&) noexcept                                        = 0;
    virtual bool destroySwapChain(SwapChain*&) noexcept                                = 0;
    virtual bool destroyCmdPool(CmdPool*&) noexcept                                    = 0;  // will destroy all cmds allocated from it automatically
    virtual bool destroyCmd(Cmd*&) noexcept                                            = 0;  // destroy the cmd individually
    virtual bool destroySampler(Sampler*&) noexcept                                    = 0;
    virtual bool destroyRenderTarget(RenderTarget*&) noexcept                          = 0;
    virtual bool destroyShader(Shader*&) noexcept                                      = 0;
};

///////////////////////////////////////////////
//                  Semaphore
///////////////////////////////////////////////
class Semaphore : public RhiObjectBase
{
public:
    virtual ~Semaphore() noexcept = default;
};

///////////////////////////////////////////////
//                  Fence
///////////////////////////////////////////////
class Fence : public RhiObjectBase
{
public:
    virtual ~Fence() noexcept                           = default;
    virtual void wait() noexcept                        = 0;
    [[nodiscard]] virtual FenceStatus status() noexcept = 0;
};

///////////////////////////////////////////////
//                  Queue
///////////////////////////////////////////////
class Queue : public RhiObjectBase
{
public:
    virtual ~Queue() noexcept                             = default;
    virtual void submit(QueueSubmitDesc& desc) noexcept   = 0;
    virtual void present(QueuePresentDesc& desc) noexcept = 0;
    virtual void waitIdle() noexcept                      = 0;
};

///////////////////////////////////////////////
//                  Swapchain
///////////////////////////////////////////////
class SwapChain : public RhiObjectBase
{
public:
    virtual ~SwapChain() noexcept                                                  = default;
    virtual void acquireNextImage(Semaphore* pSignalSemaphore, u32& outImageIndex) = 0;
    virtual void acquireNextImage(Fence* pFence, u32& outImageIndex)               = 0;
};

///////////////////////////////////////////////
//                  CmdPool
///////////////////////////////////////////////
class CmdPool : public RhiObjectBase
{
public:
    virtual ~CmdPool() noexcept   = default;
    virtual void reset() noexcept = 0;
};

///////////////////////////////////////////////
//                    Cmd
///////////////////////////////////////////////
class Cmd : public RhiObjectBase
{
public:
    virtual ~Cmd() noexcept                                                                                                           = default;
    virtual void begin() noexcept                                                                                                     = 0;
    virtual void end() noexcept                                                                                                       = 0;
    virtual void setViewport(float x, float y, float width, float height, float minDepth, float maxDepth) noexcept                    = 0;
    virtual void setScissor(u32 x, u32 y, u32 width, u32 height) noexcept                                                             = 0;
    virtual void setStencilReferenceValue(u32 val) noexcept                                                                           = 0;
    virtual void bindRenderTargets() noexcept                                                                                         = 0;
    virtual void bindDescriptorSet() noexcept                                                                                         = 0;
    virtual void bindPushConstants() noexcept                                                                                         = 0;
    virtual void bindPipeline() noexcept                                                                                              = 0;
    virtual void bindIndexBuffer() noexcept                                                                                           = 0;
    virtual void bindVertexBuffer() noexcept                                                                                          = 0;
    virtual void draw(u32 vertexCount, u32 firstIndex) noexcept                                                                       = 0;
    virtual void drawInstanced(u32 vertexCount, u32 firstIndex, u32 instanceCount, u32 firstInstance) noexcept                        = 0;
    virtual void drawIndexed(u32 indexCount, u32 firstIndex, u32 firstVertex) noexcept                                                = 0;
    virtual void drawIndexedInstanced(u32 indexCount, u32 firstIndex, u32 instanceCount, u32 firstInstance, u32 firstVertex) noexcept = 0;
    virtual void dispatch(u32 groupCountX, u32 groupCountY, u32 groupCountZ) noexcept                                                 = 0;
    virtual void resourceBarrier()                                                                                                    = 0;
    virtual void updateBuffer() noexcept                                                                                              = 0;
    virtual void updateSubresource() noexcept                                                                                         = 0;
    virtual void copySubresource() noexcept                                                                                           = 0;
    virtual void resetQueryPool() noexcept                                                                                            = 0;
    virtual void beginQuery() noexcept                                                                                                = 0;
    virtual void endQuery() noexcept                                                                                                  = 0;
    virtual void resolveQuery() noexcept                                                                                              = 0;
    virtual void addDebugMarker() noexcept                                                                                            = 0;
    virtual void beginDebugMarker() noexcept                                                                                          = 0;
    virtual void writeDebugMarker() noexcept                                                                                          = 0;
    virtual void endDebugMarker() noexcept                                                                                            = 0;
};
///////////////////////////////////////////////
//                    Sampler
///////////////////////////////////////////////
class Sampler : public RhiObjectBase
{
public:
    virtual ~Sampler() = default;
};

///////////////////////////////////////////////
//                    Texture
///////////////////////////////////////////////

class Texture : public RhiObjectBase
{
public:
    virtual ~Texture() noexcept = default;
};

///////////////////////////////////////////////
//                    RenderTarget
///////////////////////////////////////////////

class RenderTarget : public RhiObjectBase
{
public:
    virtual ~RenderTarget() noexcept = default;
};

///////////////////////////////////////////////
//                    Shader
///////////////////////////////////////////////
class Shader : public RhiObjectBase
{
};

///////////////////////////////////////////////
//                    RootSignature
///////////////////////////////////////////////

class RootSignature : public RhiObjectBase
{
};

bool create_pipeline_reflection(std::pmr::vector<ShaderReflection>& shaderRefls, PipelineReflection& outPipelineRefl) noexcept;

}  // namespace axe::rhi