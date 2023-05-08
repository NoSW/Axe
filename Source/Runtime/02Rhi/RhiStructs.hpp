#pragma once
#include "02Rhi/RhiEnums.hpp"
#include <bitset>

namespace axe::window
{
class Window;
}

namespace axe::rhi
{

//////////////////////////////////////////////////////////////////////////////////////////////
//                             GPU
//////////////////////////////////////////////////////////////////////////////////////////////
struct GPUCapBits
{
    std::bitset<TinyImageFormat_Count> canShaderReadFrom;
    std::bitset<TinyImageFormat_Count> canShaderWriteTo;
    std::bitset<TinyImageFormat_Count> canRenderTargetWriteTo;
};

struct GPUVendorPreset
{
    u32 vendorId                                                          = 0x0;
    u32 modelId                                                           = 0x0;
    u32 revisionId                                                        = 0x0;  // Optional as not all gpu's have that. Default is : 0x00
    char gpuBackendVersion[CapabilityLevel::MAX_GPU_VENDOR_STRING_LENGTH] = {0};
    char gpuName[CapabilityLevel::MAX_GPU_VENDOR_STRING_LENGTH]           = {0};
};

struct GPUSettings
{
    u32 uniformBufferAlignment             = 0;
    u32 uploadBufferTextureAlignment       = 0;
    u32 uploadBufferTextureRowAlignment    = 0;
    u32 maxVertexInputBindings             = 0;
    u32 maxRootSignatureDWORDS             = 0;
    u32 waveLaneCount                      = 0;
    WaveOpsSupportFlag waveOpsSupportFlags = WAVE_OPS_SUPPORT_FLAG_NONE;
    GPUVendorPreset gpuVendorPreset;

    // ShadingRate m_shadingRates;  // Variable Rate Shading
    // ShadingRateCaps m_shadingRateCaps;
    u32 shadingRateTexelWidth       = 0;
    u32 shadingRateTexelHeight      = 0;
    u32 timestampPeriod             = 0;

    u32 multiDrawIndirect       : 1 = 0;
    u32 ROVsSupported           : 1 = 0;
    u32 tessellationSupported   : 1 = 0;
    u32 geometryShaderSupported : 1 = 0;
    u32 gpuBreadcrumbs          : 1 = 0;
    u32 supportedHDR            : 1 = 0;
};

//////////////////////////////////////////////////////////////////////////////////////////////
//                             Object Desc
//////////////////////////////////////////////////////////////////////////////////////////////
class Backend;
class Adapter;
class Device;
class Semaphore;
class Fence;
class Queue;
class SwapChain;
class CmdPool;
class Cmd;
class Sampler;
class Texture;
class Buffer;
class RenderTarget;
class Shader;
class RootSignature;
class DescriptorSet;

struct DescBase
{
#if _DEBUG
    std::pmr::string label;
#endif
};

struct BackendDesc : public DescBase
{
    std::string_view appName = "Untitled";
};

struct AdapterDesc : public DescBase
{
    bool isSelectedBest = true;
};

struct DeviceDesc : public DescBase
{
    bool isRequestAllAvailableQueues = true;
    ShaderModel shaderModel          = SHADER_MODEL_6_7;
};

struct SemaphoreDesc : public DescBase
{
};

struct FenceDesc : public DescBase
{
    u8 isSignaled : 1 = 0;
};

struct QueueDesc : public DescBase
{
    QueueTypeFlag type     = QUEUE_TYPE_FLAG_GRAPHICS;
    QueueFlag flag         = QUEUE_FLAG_NONE;
    QueuePriority priority = QUEUE_PRIORITY_NORMAL;
};

struct QueueSubmitDesc : public DescBase
{
    std::pmr::vector<Cmd*> cmds;
    std::pmr::vector<Semaphore*> waitSemaphores;
    std::pmr::vector<Semaphore*> signalSemaphores;
    Fence* pSignalFence = nullptr;
    bool isSubmitDone   = false;
};

struct QueuePresentDesc : public DescBase
{
    std::pmr::vector<Semaphore*> waitSemaphores;
    /* TODO: SwapChain */
    void* pSwapChain  = nullptr;
    u8 index          = 0;
    bool isSubmitDone = false;
};

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

struct SwapChainDesc : public DescBase
{
    window::Window* pWindow = nullptr;
    Queue* pPresentQueue;  /// Queues which should be allowed to present
    u32 imageCount = 0;    /// Number of back buffers in this swapchain
    u32 width      = 0;
    u32 height     = 0;
    ClearValue colorClearValue;
    bool useHDR            = false;  /// Set whether swap chain will be presented using HDR
    bool enableVsync       = false;  /// Set whether swap chain will be presented using vsync
    bool useFlipSwapEffect = false;  /// We can toggle to using FLIP model if app desires.
};

struct CmdPoolDesc : public DescBase
{
    Queue* pUsedForQueue          = nullptr;
    u8 isShortLived           : 1 = 1;
    u8 isAllowIndividualReset : 1 = 1;
};

struct CmdDesc : public DescBase
{
    CmdPool* pCmdPool = nullptr;
    u32 cmdCount      = 1;
    bool isSecondary  = false;
};

struct SamplerDesc : public DescBase
{
    FilterType minFilter    = FILTER_TYPE_LINEAR;
    FilterType magFilter    = FILTER_TYPE_LINEAR;
    MipMapMode mipMapMode   = MIPMAP_MODE_LINEAR;
    AddressMode addressU    = ADDRESS_MODE_REPEAT;
    AddressMode addressV    = ADDRESS_MODE_REPEAT;
    AddressMode addressW    = ADDRESS_MODE_REPEAT;
    float mipLodBias        = 0.0f;
    float minLod            = 0.0f;
    float maxLod            = 0.0f;
    float maxAnisotropy     = 0.0f;
    bool isSetLodRange      = false;
    CompareMode compareFunc = CMP_MODE_NEVER;

    struct
    {
        TinyImageFormat format             = TinyImageFormat_UNDEFINED;
        SamplerModelConversion model       = SAMPLER_MODEL_CONVERSION_RGB_IDENTITY;
        SamplerRange range                 = SAMPLER_RANGE_FULL;
        SampleLocation chromaOffsetX       = SAMPLE_LOCATION_COSITED;
        SampleLocation chromaOffsetY       = SAMPLE_LOCATION_COSITED;
        FilterType chromaFilter            = FILTER_TYPE_LINEAR;
        bool isForceExplicitReconstruction = false;
    } samplerConversionDesc;
};

struct TextureDesc : public DescBase
{
    const void* pNativeHandle = nullptr;     // Pointer to native texture handle if the texture does not own underlying resource
    const char* pName         = "Untitled";  // Debug name used in gpu profile
    ClearValue clearValue{};                 // Optimized clear value (recommended to use this same value when clearing the render target)
    // #if defined(VULKAN)
    //     VkSamplerYcbcrConversionInfo* pVkSamplerYcbcrConversionInfo;
    // #endif

    TextureCreationFlags flags        = TEXTURE_CREATION_FLAG_NONE;  // decides memory allocation strategy, sharing access,...
    u32 width                         = 0;
    u32 height                        = 0;
    u32 depth                         = 0;  // Should be 1 if not a type is not TEXTURE_TYPE_3D)
    u32 arraySize                     = 0;  // Should be 1 if texture is not a texture array or cubemap
    u32 mipLevels                     = 0;  // number of mipmap levels
    MSAASampleCount sampleCount       = MSAA_SAMPLE_COUNT_1;
    u32 sampleQuality                 = 0;  // The valid range is between zero and the value appropriate for sampleCount
    TinyImageFormat format            = TinyImageFormat_UNDEFINED;
    ResourceStateFlags startState     = RESOURCE_STATE_UNDEFINED;   // What state will the texture get created in
    DescriptorTypeFlag descriptorType = DESCRIPTOR_TYPE_UNDEFINED;  // Descriptor creation
};

struct BufferDesc : public DescBase
{
    /// Size of the buffer (in bytes)
    u64 size                          = 0;
    /// Set this to specify a counter buffer for this buffer (applicable to BUFFER_USAGE_STORAGE_SRV, BUFFER_USAGE_STORAGE_UAV)
    Buffer* pCounterBuffer            = nullptr;
    /// Index of the first element accessible by the SRV/UAV (applicable to BUFFER_USAGE_STORAGE_SRV, BUFFER_USAGE_STORAGE_UAV)
    u64 firstElement                  = 0;
    /// Number of elements in the buffer (applicable to BUFFER_USAGE_STORAGE_SRV, BUFFER_USAGE_STORAGE_UAV)
    u64 elementCount                  = 0;
    /// Size of each element (in bytes) in the buffer (applicable to BUFFER_USAGE_STORAGE_SRV, BUFFER_USAGE_STORAGE_UAV)
    u64 structStride                  = 0;
    /// Debug name used in gpu profile
    std::string_view name             = "Untitled";
    /// Alignment
    u32 alignment                     = 0;
    /// Decides which memory heap buffer will use (default, upload, readback)
    ResourceMemoryUsage memoryUsage   = RESOURCE_MEMORY_USAGE_UNKNOWN;
    /// Creation flags of the buffer
    BufferCreationFlags flags         = BUFFER_CREATION_FLAG_NONE;
    /// What type of queue the buffer is owned by
    QueueTypeFlag queueType           = QUEUE_TYPE_FLAG_MAX;
    /// What state will the buffer get created in
    ResourceStateFlags startState     = RESOURCE_STATE_UNDEFINED;
    /// ICB draw type
    IndirectArgumentType ICBDrawType  = INDIRECT_ARG_INVALID;
    /// ICB max vertex buffers slots count
    u32 ICBMaxCommandCount            = 0;
    /// Format of the buffer (applicable to typed storage buffers (Buffer<T>)
    TinyImageFormat format            = TinyImageFormat_UNDEFINED;
    /// Flags specifying the suitable usage of this buffer (Uniform buffer, Vertex Buffer, Index Buffer,...)
    DescriptorTypeFlag descriptorType = DESCRIPTOR_TYPE_UNDEFINED;
};

struct RenderTargetDesc : public DescBase
{
    TextureCreationFlags flags       = TEXTURE_CREATION_FLAG_NONE;  // decides memory allocation strategy, sharing access,...
    u32 width                        = 0;
    u32 height                       = 0;
    u32 depth                        = 1;  // (Should be 1 if not a type is not TEXTURE_TYPE_3D)
    u32 arraySize                    = 1;  // Texture array size (Should be 1 if texture is not a texture array or cubemap)
    u32 mipLevels                    = 0;  // Number of mip levels
    MSAASampleCount mMSAASampleCount = MSAA_SAMPLE_COUNT_1;
    TinyImageFormat format           = TinyImageFormat_UNDEFINED;   // Internal image format
    ResourceStateFlags startState    = RESOURCE_STATE_UNDEFINED;    // What flag will the texture get created in
    ClearValue clearValue{.rgba = {0, 0, 0, 0}};                    // Optimized clear value (recommended to use this same value when clearing the RenderTarget)
    u32 sampleQuality                 = 1;                          // The image quality level. The higher the quality, the lower the performance. The valid range is between zero and the value appropriate for sampleCount
    DescriptorTypeFlag descriptorType = DESCRIPTOR_TYPE_UNDEFINED;  // Descriptor creation
    const void* mpNativeHandle        = nullptr;
    const char* mpName                = "Untitled";  // Debug name used in gpu profile
};

struct ShaderStageDesc
{
    ShaderStageFlagOneBit mStage = SHADER_STAGE_FLAG_NONE;
    std::string_view mRelaFilePath;  // **relative** glsl filepath under Source/, e.g. Shaders/Basic.vert.glsl
    std::string_view mEntryPoint = "main";
    ShaderStageLoadFlag flags    = SHADER_STAGE_LOAD_FLAG_NONE;
};

struct ShaderConstants  // only supported by Vulkan APIs
{
    std::pmr::vector<u8> mBlob;
    u32 index;
};

struct ShaderDesc : public DescBase
{
    std::pmr::vector<ShaderStageDesc> mStages;
    std::pmr::vector<ShaderConstants> mConstants;
    ShaderModel mShaderMode = SHADER_MODEL_6_7;
};

struct RootSignatureDesc : public DescBase
{
    std::pmr::vector<Shader*> mShaders;
    std::pmr::unordered_map<std::string_view, Sampler*> mStaticSamplersMap;  // <unique name, pointer>
    u32 mMaxBindlessTextures = 0;
    RootSignatureFlags flags = ROOT_SIGNATURE_FLAG_NONE;
};

struct DescriptorSetDesc : public DescBase
{
    RootSignature* mpRootSignature            = nullptr;
    u32 mMaxSet                               = 0;  // the number of descriptor sets want to allocate
    DescriptorUpdateFrequency updateFrequency = DESCRIPTOR_UPDATE_FREQ_NONE;
};

struct DescriptorInfo
{
    std::string_view name;
    u32 type;
    u32 size;
    u32 handleIndex;
    u32 dim              : 4;
    u32 isRootDescriptor : 1;
    u32 isStaticSampler  : 1;
    u32 updateFrequency  : 3;

    u32 vkStages         : 8;
    u32 vkType;
    u32 reg;
};

struct DescriptorDataRange
{
    u32 offset;
    u32 size;
};

struct DescriptorData
{
    std::string_view name;                   /// Name of descriptor
    u32 arrayOffset              = 0;        /// Offset in array to update (starting element to update)
    u32 index                    = U32_MAX;  // Index in pRootSignature->pDescriptors array - Cache index using getDescriptorIndexFromName to avoid using string checks at runtime
    bool isBindStencilResource   = false;    // Binds stencil only descriptor instead of color/depth
    DescriptorDataRange* pRanges = nullptr;  // Range to bind (buffer offset, size)

    /// Array of resources containing descriptor handles or constant to be used in ring buffer memory - DescriptorRange can hold only one resource type array
    std::pmr::vector<void*> pResources;  // (srv/uav)textures, samplers, (srv/uav/cbv)buffers, other custom resources(e.g., acceleration structure)

    union
    {
        struct
        {
            u16 UAVMipSlice;      // When binding UAV, control the mip slice to to bind for UAV (example - generating mipmaps in a compute shader)
            bool isBindMipChain;  // Binds entire mip chain as array of UAV
        };
        struct
        {
            // Bind MTLIndirectCommandBuffer along with the MTLBuffer
            const char* ICBName;  // TODO: type with constructor in anonymous aggregate is NOT allowed when using gcc
            u32 ICBIndex;
            bool bindICB;
        };
    };
};

struct NullDescriptors
{
    std::array<Texture*, TEXTURE_DIM_COUNT> pDefaultTextureSRV{};
    std::array<Texture*, TEXTURE_DIM_COUNT> pDefaultTextureUAV{};
    Buffer* pDefaultBufferSRV          = nullptr;
    Buffer* pDefaultBufferUAV          = nullptr;
    Sampler* pDefaultSampler           = nullptr;
    // Mutex submitMutex;

    // #TODO - Remove after we have a better way to specify initial resource state
    // Unlike DX12, Vulkan textures start in undefined layout.
    // With this, we transition them to the specified layout so app code doesn't have to worry about this
    // Mutex mInitialTransitionMutex;
    Queue* pInitialTransitionQueue     = nullptr;
    CmdPool* pInitialTransitionCmdPool = nullptr;
    Cmd* pInitialTransitionCmd         = nullptr;
    Fence* pInitialTransitionFence     = nullptr;
};

// Raster
struct BlendStateDesc
{
    struct
    {
        BlendConstant srcFactor      = BC_ONE;
        BlendConstant dstFactor      = BC_ZERO;
        BlendConstant srcAlphaFactor = BC_ONE;
        BlendConstant dstAlphaFactor = BC_ZERO;
        BlendMode blendMode          = BM_ADD;
        BlendMode blendAlphaMode     = BM_ADD;
        i32 mask                     = CH_ALL;
    } perRenderTarget[MAX_RENDER_TARGET_ATTACHMENTS];

    u8 attachmentCount                     = 0;                       /// The render target attachment to set the blend state for.
    BlendStateTargetsFlag renderTargetMask = BLEND_STATE_TARGET_ALL;  /// Mask that identifies the render targets affected by the blend state.
    bool isAlphaToCoverage                 = false;                   /// Set whether alpha to coverage should be enabled.
    bool isIndependentBlend                = false;                   /// Set whether each render target has an unique blend function.
                                                                      /// When false the blend function in slot 0 will be used for all render targets.
};

// Barrier

struct BarrierInfo
{
    ResourceStateFlags currentState = RESOURCE_STATE_UNDEFINED;
    ResourceStateFlags newState     = RESOURCE_STATE_UNDEFINED;
    u8 isBeginOnly : 1              = 0;
    u8 isEndOnly   : 1              = 0;
    u8 isAcquire   : 1              = 0;
    u8 isRelease   : 1              = 0;
    u8 queueType   : 5              = 0;
};

struct ImageBarrier
{
    BarrierInfo barrierInfo{};
    u8 isSubresourceBarrier : 1 = 0;  // Specify whether following barrier targets particular subresource
    u8 mipLevel             : 7 = 0;  // ignored if isSubresourceBarrier is false
    u16 arrayLayer              = 0;  // ignored if isSubresourceBarrier is false
};

struct TextureBarrier
{
    ImageBarrier imageBarrier{};
    Texture* pTexture = nullptr;
};

struct RenderTargetBarrier
{
    ImageBarrier imageBarrier{};
    RenderTarget* pRenderTarget = nullptr;
};

struct BufferBarrier
{
    BarrierInfo barrierInfo{};
    Buffer* pBuffer = nullptr;
};

}  // namespace axe::rhi
