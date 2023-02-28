/* All public interfaces of 02Rhi */
#pragma once

#include "02Rhi/Gpu.hpp"
#include "02Rhi/Config.hpp"
#include <00Core/Log/Log.hpp>
#include <00Core/Reflection/Reflection.hpp>

namespace axe::window
{
class Window;
}

namespace axe::rhi
{
class Driver;
///////////////////////////////////////////////
//                  Semaphore
///////////////////////////////////////////////
struct SemaphoreDesc
{
};

class Semaphore
{
public:
    virtual ~Semaphore() noexcept = default;
};

///////////////////////////////////////////////
//                  Fence
///////////////////////////////////////////////
enum FenceStatus
{
    FENCE_STATUS_COMPLETE = 0,
    FENCE_STATUS_INCOMPLETE,
    FENCE_STATUS_NOTSUBMITTED,
};

struct FenceDesc
{
    u8 mIsSignaled : 1 = 0;
};

class Fence
{
public:
    virtual ~Fence() noexcept                           = default;
    virtual void wait() noexcept                        = 0;
    [[nodiscard]] virtual FenceStatus status() noexcept = 0;
};

///////////////////////////////////////////////
//                  Queue
///////////////////////////////////////////////
class Cmd;
enum QueueType
{
    QUEUE_TYPE_GRAPHICS = 0,
    QUEUE_TYPE_COMPUTE,
    QUEUE_TYPE_TRANSFER,
    MAX_QUEUE_TYPE
};

enum QueueFlag
{
    QUEUE_FLAG_NONE                = 0x0,
    QUEUE_FLAG_DISABLE_GPU_TIMEOUT = 0x1,
    QUEUE_FLAG_INIT_MICROPROFILE   = 0x2,
    MAX_QUEUE_FLAG                 = 0xFFFFFFFF
};

enum QueuePriority
{
    QUEUE_PRIORITY_NORMAL,
    QUEUE_PRIORITY_HIGH,
    QUEUE_PRIORITY_GLOBAL_REALTIME,
    MAX_QUEUE_PRIORITY
};

struct QueueDesc
{
    u32 mNodeIndex          = 0;
    QueueType mType         = QUEUE_TYPE_GRAPHICS;
    QueueFlag mFlag         = QUEUE_FLAG_NONE;
    QueuePriority mPriority = QUEUE_PRIORITY_NORMAL;
};

struct QueueSubmitDesc
{
    std::pmr::vector<Cmd*> mCmds;
    std::pmr::vector<Semaphore*> mWaitSemaphores;
    std::pmr::vector<Semaphore*> mSignalSemaphores;
    Fence* mpSignalFence = nullptr;
    bool mSubmitDone     = false;
};

struct QueuePresentDesc
{
    std::pmr::vector<Semaphore*> mWaitSemaphores;
    /* TODO: SwapChain */
    void* mpSwapChain = nullptr;
    u8 mIndex         = 0;
    bool mSubmitDone  = false;
};

class Queue
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

union ClearValue
{
    struct
    {
        float r, g, b, a;
    } rgba;
    struct
    {
        float depth;
        u32 stencil;
    } depthStencil;
};

struct SwapChainDesc
{
    window::Window* mpWindow = nullptr;
    std::pmr::vector<Queue*> mpPresentQueues;  /// Queues which should be allowed to present
    u32 mImageCount = 0;                       /// Number of back buffers in this swapchain
    u32 mWidth      = 0;
    u32 mHeight     = 0;
    ClearValue mColorClearValue;
    bool mEnableVsync       = false;  /// Set whether swap chain will be presented using vsync
    bool mUseFlipSwapEffect = false;  /// We can toggle to using FLIP model if app desires.
    // TinyImageFormat mColorFormat;    /// Color format of the swapchain
};

class SwapChain
{
public:
    virtual ~SwapChain() noexcept                                                  = default;
    virtual void acquireNextImage(Semaphore* pSignalSemaphore, u32& outImageIndex) = 0;
    virtual void acquireNextImage(Fence* pFence, u32& outImageIndex)               = 0;
};

///////////////////////////////////////////////
//                  CmdPool
///////////////////////////////////////////////
struct CmdPoolDesc
{
    Queue* mpUsedForQueue        = nullptr;
    u8 mShortLived           : 1 = 1;
    u8 mAllowIndividualReset : 1 = 1;
};

class CmdPool
{
public:
    virtual ~CmdPool() noexcept   = default;
    virtual void reset() noexcept = 0;
};

///////////////////////////////////////////////
//                    Cmd
///////////////////////////////////////////////
struct CmdDesc
{
    CmdPool* mpCmdPool = nullptr;
    u32 mCmdCount      = 1;
    bool mSecondary    = false;
};

class Cmd
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
//                    Texture
///////////////////////////////////////////////
enum ResourceState
{
    RESOURCE_STATE_UNDEFINED                         = 0,
    RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER        = 0x1,
    RESOURCE_STATE_INDEX_BUFFER                      = 0x2,
    RESOURCE_STATE_RENDER_TARGET                     = 0x4,
    RESOURCE_STATE_UNORDERED_ACCESS                  = 0x8,
    RESOURCE_STATE_DEPTH_WRITE                       = 0x10,
    RESOURCE_STATE_DEPTH_READ                        = 0x20,
    RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE         = 0x40,
    RESOURCE_STATE_PIXEL_SHADER_RESOURCE             = 0x80,
    RESOURCE_STATE_SHADER_RESOURCE                   = 0x40 | 0x80,
    RESOURCE_STATE_STREAM_OUT                        = 0x100,
    RESOURCE_STATE_INDIRECT_ARGUMENT                 = 0x200,
    RESOURCE_STATE_COPY_DEST                         = 0x400,
    RESOURCE_STATE_COPY_SOURCE                       = 0x800,
    RESOURCE_STATE_GENERIC_READ                      = (((((0x1 | 0x2) | 0x40) | 0x80) | 0x200) | 0x800),
    RESOURCE_STATE_PRESENT                           = 0x1000,
    RESOURCE_STATE_COMMON                            = 0x2000,
    RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE = 0x4000,
    RESOURCE_STATE_SHADING_RATE_SOURCE               = 0x8000,
};

enum DescriptorType
{
    DESCRIPTOR_TYPE_UNDEFINED                  = 0,
    DESCRIPTOR_TYPE_SAMPLER                    = 0x01,
    // SRV Read only texture
    DESCRIPTOR_TYPE_TEXTURE                    = (DESCRIPTOR_TYPE_SAMPLER << 1),
    /// UAV Texture
    DESCRIPTOR_TYPE_RW_TEXTURE                 = (DESCRIPTOR_TYPE_TEXTURE << 1),
    // SRV Read only buffer
    DESCRIPTOR_TYPE_BUFFER                     = (DESCRIPTOR_TYPE_RW_TEXTURE << 1),
    DESCRIPTOR_TYPE_BUFFER_RAW                 = (DESCRIPTOR_TYPE_BUFFER | (DESCRIPTOR_TYPE_BUFFER << 1)),
    /// UAV Buffer
    DESCRIPTOR_TYPE_RW_BUFFER                  = (DESCRIPTOR_TYPE_BUFFER << 2),
    DESCRIPTOR_TYPE_RW_BUFFER_RAW              = (DESCRIPTOR_TYPE_RW_BUFFER | (DESCRIPTOR_TYPE_RW_BUFFER << 1)),
    /// Uniform buffer
    DESCRIPTOR_TYPE_UNIFORM_BUFFER             = (DESCRIPTOR_TYPE_RW_BUFFER << 2),
    /// Push constant / Root constant
    DESCRIPTOR_TYPE_ROOT_CONSTANT              = (DESCRIPTOR_TYPE_UNIFORM_BUFFER << 1),
    /// IA
    DESCRIPTOR_TYPE_VERTEX_BUFFER              = (DESCRIPTOR_TYPE_ROOT_CONSTANT << 1),
    DESCRIPTOR_TYPE_INDEX_BUFFER               = (DESCRIPTOR_TYPE_VERTEX_BUFFER << 1),
    DESCRIPTOR_TYPE_INDIRECT_BUFFER            = (DESCRIPTOR_TYPE_INDEX_BUFFER << 1),
    /// Cubemap SRV
    DESCRIPTOR_TYPE_TEXTURE_CUBE               = (DESCRIPTOR_TYPE_TEXTURE | (DESCRIPTOR_TYPE_INDIRECT_BUFFER << 1)),
    /// RTV / DSV per mip slice
    DESCRIPTOR_TYPE_RENDER_TARGET_MIP_SLICES   = (DESCRIPTOR_TYPE_INDIRECT_BUFFER << 2),
    /// RTV / DSV per array slice
    DESCRIPTOR_TYPE_RENDER_TARGET_ARRAY_SLICES = (DESCRIPTOR_TYPE_RENDER_TARGET_MIP_SLICES << 1),
    /// RTV / DSV per depth slice
    DESCRIPTOR_TYPE_RENDER_TARGET_DEPTH_SLICES = (DESCRIPTOR_TYPE_RENDER_TARGET_ARRAY_SLICES << 1),
    DESCRIPTOR_TYPE_RAY_TRACING                = (DESCRIPTOR_TYPE_RENDER_TARGET_DEPTH_SLICES << 1),
    DESCRIPTOR_TYPE_INDIRECT_COMMAND_BUFFER    = (DESCRIPTOR_TYPE_RAY_TRACING << 1),
#if AXE_02RHI_API_USED_VULKAN
    /// Subpass input (descriptor type only available in Vulkan)
    DESCRIPTOR_TYPE_INPUT_ATTACHMENT                   = (DESCRIPTOR_TYPE_INDIRECT_COMMAND_BUFFER << 1),
    DESCRIPTOR_TYPE_TEXEL_BUFFER                       = (DESCRIPTOR_TYPE_INPUT_ATTACHMENT << 1),
    DESCRIPTOR_TYPE_RW_TEXEL_BUFFER                    = (DESCRIPTOR_TYPE_TEXEL_BUFFER << 1),
    DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER             = (DESCRIPTOR_TYPE_RW_TEXEL_BUFFER << 1),

    /// Khronos extension ray tracing
    DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE             = (DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER << 1),
    DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_BUILD_INPUT = (DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE << 1),
    DESCRIPTOR_TYPE_SHADER_DEVICE_ADDRESS              = (DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_BUILD_INPUT << 1),
    DESCRIPTOR_TYPE_SHADER_BINDING_TABLE               = (DESCRIPTOR_TYPE_SHADER_DEVICE_ADDRESS << 1),
#endif
};

enum TextureCreationFlags
{
    // Default flag (Texture will use default allocation strategy decided by the api specific allocator)
    TEXTURE_CREATION_FLAG_NONE                      = 0,
    // Texture will allocate its own memory (COMMITTED resource)
    TEXTURE_CREATION_FLAG_OWN_MEMORY_BIT            = 0x01,
    // Texture will be allocated in memory which can be shared among multiple processes
    TEXTURE_CREATION_FLAG_EXPORT_BIT                = 0x02,
    // Texture will be allocated in memory which can be shared among multiple gpus
    TEXTURE_CREATION_FLAG_EXPORT_ADAPTER_BIT        = 0x04,
    // Texture will be imported from a handle created in another process
    TEXTURE_CREATION_FLAG_IMPORT_BIT                = 0x08,
    // Use ESRAM to store this texture
    TEXTURE_CREATION_FLAG_ESRAM                     = 0x10,
    // Use on-tile memory to store this texture
    TEXTURE_CREATION_FLAG_ON_TILE                   = 0x20,
    // Prevent compression meta data from generating (XBox)
    TEXTURE_CREATION_FLAG_NO_COMPRESSION            = 0x40,
    // Force 2D instead of automatically determining dimension based on width, height, depth
    TEXTURE_CREATION_FLAG_FORCE_2D                  = 0x80,
    // Force 3D instead of automatically determining dimension based on width, height, depth
    TEXTURE_CREATION_FLAG_FORCE_3D                  = 0x100,
    // Display target
    TEXTURE_CREATION_FLAG_ALLOW_DISPLAY_TARGET      = 0x200,
    // Create an sRGB texture.
    TEXTURE_CREATION_FLAG_SRGB                      = 0x400,
    // Create a normal map texture
    TEXTURE_CREATION_FLAG_NORMAL_MAP                = 0x800,
    // Fast clear
    TEXTURE_CREATION_FLAG_FAST_CLEAR                = 0x1000,
    // Fragment mask
    TEXTURE_CREATION_FLAG_FRAG_MASK                 = 0x2000,
    // Doubles the amount of array layers of the texture when rendering VR. Also forces the texture to be a 2D Array texture.
    TEXTURE_CREATION_FLAG_VR_MULTIVIEW              = 0x4000,
    // Binds the FFR fragment density if this texture is used as a render target.
    TEXTURE_CREATION_FLAG_VR_FOVEATED_RENDERING     = 0x8000,
    // Creates resolve attachment for auto resolve (MSAA on tiled architecture - Resolve can be done on tile through render pass)
    TEXTURE_CREATION_FLAG_CREATE_RESOLVE_ATTACHMENT = 0x10000,
};

enum MSAASampleCount
{
    MSAA_SAMPLE_COUNT_1     = 1,
    MSAA_SAMPLE_COUNT_2     = 2,
    MSAA_SAMPLE_COUNT_4     = 4,
    MSAA_SAMPLE_COUNT_8     = 8,
    MSAA_SAMPLE_COUNT_16    = 16,
    MSAA_SAMPLE_COUNT_COUNT = 5,
};

struct TextureDesc
{
    ClearValue mClearValue;     // Optimized clear value (recommended to use this same value when clearing the render target)
    const void* pNativeHandle;  // Pointer to native texture handle if the texture does not own underlying resource
    const char* pName;          // Debug name used in gpu profile
    u32* pSharedNodeIndices;    // GPU indices to share this texture
    // #if defined(VULKAN)
    //     VkSamplerYcbcrConversionInfo* pVkSamplerYcbcrConversionInfo;
    // #endif

    TextureCreationFlags mFlags;  // decides memory allocation strategy, sharing access,...
    u32 mWidth;
    u32 mHeight;
    u32 mDepth;      // Should be 1 if not a mType is not TEXTURE_TYPE_3D)
    u32 mArraySize;  // Should be 1 if texture is not a texture array or cubemap
    u32 mMipLevels;  // number of mipmap levels
    MSAASampleCount mSampleCount;
    u32 mSampleQuality;  // The valid range is between zero and the value appropriate for mSampleCount
    // TinyImageFormat mFormat;
    ResourceState mStartState;    // What state will the texture get created in
    DescriptorType mDescriptors;  // Descriptor creation
    u32 mSharedNodeIndexCount;    // Number of GPUs to share this texture
    u32 mNodeIndex;               // GPU which will own this texture
};

class Texture
{
public:
    virtual ~Texture() noexcept = default;
};

///////////////////////////////////////////////
//                    RenderTarget
///////////////////////////////////////////////

enum ResourceFlag
{
    RESOURCE_FLAG_UNDEFINED                         = 0,
    RESOURCE_FLAG_VERTEX_AND_CONSTANT_BUFFER        = (1 << 0),
    RESOURCE_FLAG_INDEX_BUFFER                      = (1 << 1),
    RESOURCE_FLAG_RENDER_TARGET                     = (1 << 2),
    RESOURCE_FLAG_UNORDERED_ACCESS                  = (1 << 3),
    RESOURCE_FLAG_DEPTH_WRITE                       = (1 << 4),
    RESOURCE_FLAG_DEPTH_READ                        = (1 << 5),
    RESOURCE_FLAG_NON_PIXEL_SHADER_RESOURCE         = (1 << 6),
    RESOURCE_FLAG_PIXEL_SHADER_RESOURCE             = (1 << 7),
    RESOURCE_FLAG_STREAM_OUT                        = (1 << 8),
    RESOURCE_FLAG_INDIRECT_ARGUMENT                 = (1 << 9),
    RESOURCE_FLAG_COPY_DEST                         = (1 << 10),
    RESOURCE_FLAG_COPY_SOURCE                       = (1 << 11),
    RESOURCE_FLAG_PRESENT                           = (1 << 12),
    RESOURCE_FLAG_COMMON                            = (1 << 13),
    RESOURCE_FLAG_RAYTRACING_ACCELERATION_STRUCTURE = (1 << 14),
    RESOURCE_FLAG_SHADING_RATE_SOURCE               = (1 << 15),
    RESOURCE_FLAG_SHADER_RESOURCE                   = RESOURCE_FLAG_NON_PIXEL_SHADER_RESOURCE | RESOURCE_FLAG_PIXEL_SHADER_RESOURCE,
    RESOURCE_FLAG_GENERIC_READ                      = RESOURCE_FLAG_VERTEX_AND_CONSTANT_BUFFER | RESOURCE_FLAG_INDEX_BUFFER |
                                 RESOURCE_FLAG_NON_PIXEL_SHADER_RESOURCE | RESOURCE_FLAG_PIXEL_SHADER_RESOURCE |
                                 RESOURCE_FLAG_INDIRECT_ARGUMENT | RESOURCE_FLAG_COPY_SOURCE,
};

struct RenderTargetDesc
{
    // TextureCreationFlags mFlags; // decides memory allocation strategy, sharing access,...
    u32 mWidth;
    u32 mHeight;
    u32 mDepth;      // (Should be 1 if not a mType is not TEXTURE_TYPE_3D)
    u32 mArraySize;  // Texture array size (Should be 1 if texture is not a texture array or cubemap)
    u32 mMipLevels;  // Number of mip levels
    MSAASampleCount mMSAASampleCount;
    // TinyImageFormat mFormat;  // Internal image format
    ResourceFlag mStartState;  // What flag will the texture get created in
    ClearValue mClearValue;    // Optimized clear value (recommended to use this same value when clearing the RenderTarget)
    u32 mSampleQuality;        // The image quality level. The higher the quality, the lower the performance. The valid range is between zero and the value appropriate for mSampleCount
    // DescriptorType mDescriptors; // Descriptor creation
    const void* pNativeHandle;
    const char* pName;          // Debug name used in gpu profile
    u32* pSharedNodeIndices;    // GPU indices to share this texture
    u32 mSharedNodeIndexCount;  // Number of GPUs to share this texture
    u32 mNodeIndex;             // GPU which will own this texture
};

class RenderTarget
{
public:
    virtual ~RenderTarget() noexcept = default;
};

struct RenderTargetBarrier
{
    RenderTarget* mpRenderTarget = nullptr;
    ResourceFlag mCurrentFlag    = RESOURCE_FLAG_UNDEFINED;
    ResourceFlag mNewFlag        = RESOURCE_FLAG_UNDEFINED;
    u32 mBeginOnly          : 1  = 0;
    u32 mEndOnly            : 1  = 0;
    u32 mAcquire            : 1  = 0;
    u32 mRelease            : 1  = 0;
    u32 mQueueType          : 5  = 0;
    u32 mSubresourceBarrier : 1  = 0;  // Specifics whether following barrier targets particular subresource
    u32 mMipLevel           : 7  = 0;  // ignored if mSubresourceBarrier is false
    u32 mArrayLayer              = 0;  // ignored if mSubresourceBarrier is false
};

///////////////////////////////////////////////
//                    Driver
///////////////////////////////////////////////
struct DriverDesc
{
    const char* m_appName = nullptr;
    GpuMode mGpuMode      = GPU_MODE_LINKED;
};

enum GraphicsApi
{
    GRAPHICS_API_NULL      = AXE_02RHI_API_FLAG_NULL,
    GRAPHICS_API_VULKAN    = AXE_02RHI_API_FLAG_VULKAN,
    GRAPHICS_API_D3D12     = AXE_02RHI_API_FLAG_D3D12,
    GRAPHICS_API_METAL     = AXE_02RHI_API_FLAG_METAL,
    GRAPHICS_API_AVAILABLE = AXE_02RHI_API_FLAG_AVAILABLE,
};

class VulkanDriver;
class D3D12Driver;
class Driver
{
public:
    virtual ~Driver() noexcept                                                         = default;

    // All available rhi interfaces:
    virtual bool init(DriverDesc&) noexcept                                            = 0;
    virtual void exit() noexcept                                                       = 0;

    [[nodiscard]] virtual Semaphore* createSemaphore(SemaphoreDesc&) noexcept          = 0;
    [[nodiscard]] virtual Fence* createFence(FenceDesc&) noexcept                      = 0;
    [[nodiscard]] virtual Queue* createQueue(QueueDesc&) noexcept                      = 0;
    [[nodiscard]] virtual SwapChain* createSwapChain(SwapChainDesc&) noexcept          = 0;
    [[nodiscard]] virtual CmdPool* createCmdPool(CmdPoolDesc&) noexcept                = 0;
    [[nodiscard]] virtual Cmd* createCmd(CmdDesc&) noexcept                            = 0;
    [[nodiscard]] virtual RenderTarget* createRenderTarget(RenderTargetDesc&) noexcept = 0;
    virtual bool destroySemaphore(Semaphore*&) noexcept                                = 0;
    virtual bool destroyFence(Fence*&) noexcept                                        = 0;
    virtual bool destroyQueue(Queue*&) noexcept                                        = 0;
    virtual bool destroySwapChain(SwapChain*&) noexcept                                = 0;
    virtual bool destroyCmdPool(CmdPool*&) noexcept                                    = 0;  // will destroy all cmds allocated from it automatically
    virtual bool destroyCmd(Cmd*&) noexcept                                            = 0;  // destroy the cmd individually
    virtual bool destroyRenderTarget(RenderTarget*&) noexcept                          = 0;
};

}  // namespace axe::rhi