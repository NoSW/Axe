#pragma once
#include "02Rhi/RhiEnums.hpp"
#include <bitset>
#include <span>

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
    WaveOpsSupportFlag waveOpsSupportFlags = WaveOpsSupportFlag::NONE;
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
class Pipeline;

struct DescBase
{
#if _DEBUG
private:
    std::pmr::string label;

public:
    // only do actual work in Debug, do nothing in Release
    void setLabel_DebugActiveOnly(std::string_view s) noexcept { label = s; }

    // return label in Debug, return null string in Release
    std::string_view getLabel_DebugActiveOnly() const noexcept { return label; }
#else
    // only do actual work in Debug, do nothing in Release
    void setLabel_DebugActiveOnly(std::string_view s) noexcept
    { /* do nothing */
    }

    // return label in Debug, return null string in Release
    std::string_view getLabel_DebugActiveOnly() const noexcept { return {}; }

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
    ShaderModel shaderModel          = ShaderModel::SM_6_7;
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
    QueueTypeFlag type     = QueueTypeFlag::GRAPHICS;
    QueueFlag flag         = QueueFlag::NONE;
    QueuePriority priority = QueuePriority::NORMAL;
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
    FilterType minFilter    = FilterType::LINEAR;
    FilterType magFilter    = FilterType::LINEAR;
    MipMapMode mipMapMode   = MipMapMode::LINEAR;
    AddressMode addressU    = AddressMode::REPEAT;
    AddressMode addressV    = AddressMode::REPEAT;
    AddressMode addressW    = AddressMode::REPEAT;
    float mipLodBias        = 0.0f;
    float minLod            = 0.0f;
    float maxLod            = 0.0f;
    float maxAnisotropy     = 0.0f;
    bool isSetLodRange      = false;
    CompareMode compareFunc = CompareMode::NEVER;

    struct
    {
        TinyImageFormat format             = TinyImageFormat_UNDEFINED;
        SamplerModelConversion model       = SamplerModelConversion::RGB_IDENTITY;
        SamplerRange range                 = SamplerRange::FULL;
        SampleLocation chromaOffsetX       = SampleLocation::COSITED;
        SampleLocation chromaOffsetY       = SampleLocation::COSITED;
        FilterType chromaFilter            = FilterType::LINEAR;
        bool isForceExplicitReconstruction = false;
    } samplerConversionDesc;
};

struct TextureDesc : public DescBase
{
    nullable<const void> pNativeHandle = nullptr;     // Pointer to native texture handle if the texture does not own underlying resource
    std::string_view pName             = "Untitled";  // Debug name used in gpu profile
    ClearValue clearValue{};                          // Optimized clear value (recommended to use this same value when clearing the render target)
    // #if defined(VULKAN)
    //     VkSamplerYcbcrConversionInfo* pVkSamplerYcbcrConversionInfo;
    // #endif

    TextureCreationFlags flags        = TextureCreationFlags::NONE;  // decides memory allocation strategy, sharing access,...
    u32 width                         = 0;
    u32 height                        = 0;
    u32 depth                         = 0;  // Should be 1 if not a type is not TEXTURE_TYPE_3D)
    u32 arraySize                     = 0;  // Should be 1 if texture is not a texture array or cubemap
    u32 mipLevels                     = 0;  // number of mipmap levels
    MSAASampleCount sampleCount       = MSAASampleCount::COUNT_1;
    u32 sampleQuality                 = 0;  // The valid range is between zero and the value appropriate for sampleCount
    TinyImageFormat format            = TinyImageFormat_UNDEFINED;
    ResourceStateFlags startState     = ResourceStateFlags::UNDEFINED;  // What state will the texture get created in
    DescriptorTypeFlag descriptorType = DescriptorTypeFlag::UNDEFINED;  // Descriptor creation
};

struct TextureUpdateDesc
{
    rhi::Buffer* pSrcBuffer = nullptr;
    rhi::Cmd* pCmd          = nullptr;
    u32 baseMipLevel        = 0;
    u32 mipLevels           = 0;
    u32 baseArrayLayer      = 0;
    u32 layerCount          = 0;
};

struct SubresourceDataDesc
{
    u64 srcOffset;
    u32 mipLevel;
    u32 arrayLayer;
    u32 rowPitch;
    u32 slicePitch;
};

struct BufferDesc : public DescBase
{
    u64 size                          = 0;                             // (in bytes)
    u32 alignment                     = 0;                             // alignment of the buffer (in bytes)
    ResourceMemoryUsage memoryUsage   = ResourceMemoryUsage::UNKNOWN;  // Decides which memory heap buffer will use (default, upload, readback)
    BufferCreationFlags flags         = BufferCreationFlags::NONE;     // create flags

    /// Set this to specify a counter buffer for this buffer (applicable to BUFFER_USAGE_STORAGE_SRV, BUFFER_USAGE_STORAGE_UAV)
    Buffer* pCounterBuffer            = nullptr;
    /// Index of the first element accessible by the SRV/UAV (applicable to BUFFER_USAGE_STORAGE_SRV, BUFFER_USAGE_STORAGE_UAV)
    u64 firstElement                  = 0;
    /// Number of elements in the buffer (applicable to BUFFER_USAGE_STORAGE_SRV, BUFFER_USAGE_STORAGE_UAV)
    u64 elementCount                  = 0;
    /// Size of each element (in bytes) in the buffer (applicable to BUFFER_USAGE_STORAGE_SRV, BUFFER_USAGE_STORAGE_UAV)
    u64 structStride                  = 0;

    /// What type of queue the buffer is owned by
    QueueTypeFlag queueType           = QueueTypeFlag::UNDEFINED;
    /// What state will the buffer get created in
    ResourceStateFlags startState     = ResourceStateFlags::UNDEFINED;
    /// ICB draw type
    IndirectArgumentType ICBDrawType  = IndirectArgumentType::INVALID;
    /// ICB max vertex buffers slots count
    u32 ICBMaxCommandCount            = 0;
    /// Format of the buffer (applicable to typed storage buffers (Buffer<T>)
    TinyImageFormat format            = TinyImageFormat_UNDEFINED;
    /// Flags specifying the suitable usage of this buffer (Uniform buffer, Vertex Buffer, Index Buffer,...)
    DescriptorTypeFlag descriptorType = DescriptorTypeFlag::UNDEFINED;
};

struct RenderTargetDesc : public DescBase
{
    TextureCreationFlags flags       = TextureCreationFlags::NONE;  // decides memory allocation strategy, sharing access,...
    u32 width                        = 0;
    u32 height                       = 0;
    u32 depth                        = 1;  // (Should be 1 if not a type is not TEXTURE_TYPE_3D)
    u32 arraySize                    = 1;  // Texture array size (Should be 1 if texture is not a texture array or cubemap)
    u32 mipLevels                    = 0;  // Number of mip levels
    MSAASampleCount mMSAASampleCount = MSAASampleCount::COUNT_1;
    TinyImageFormat format           = TinyImageFormat_UNDEFINED;        // Internal image format
    ResourceStateFlags startState    = ResourceStateFlags::UNDEFINED;    // What flag will the texture get created in
    ClearValue clearValue{.rgba = {0, 0, 0, 0}};                         // Optimized clear value (recommended to use this same value when clearing the RenderTarget)
    u32 sampleQuality                  = 1;                              // The image quality level. The higher the quality, the lower the performance. The valid range is between zero and the value appropriate for sampleCount
    DescriptorTypeFlag descriptorType  = DescriptorTypeFlag::UNDEFINED;  // Descriptor creation
    nullable<const void> pNativeHandle = nullptr;
    std::string_view name              = "Untitled";
};

struct ShaderStageDesc
{
    std::string_view name        = "Untitled";
    ShaderStageFlagOneBit mStage = ShaderStageFlag::NONE;
    std::array<std::span<u8>, GRAPHICS_API_COUNT> byteCode;
    std::string_view mEntryPoint = "main";
    ShaderStageLoadFlag flags    = ShaderStageLoadFlag::NONE;
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
    ShaderModel mShaderMode = ShaderModel::SM_6_7;
};

struct RootSignatureDesc : public DescBase
{
    std::pmr::vector<Shader*> mShaders;
    std::pmr::unordered_map<std::string_view, Sampler*> mStaticSamplersMap;  // <unique name, pointer>
    u32 mMaxBindlessTextures = 0;
    RootSignatureFlags flags = RootSignatureFlags::NONE;
};

struct DescriptorSetDesc : public DescBase
{
    RootSignature* mpRootSignature            = nullptr;
    u32 mMaxSet                               = 0;  // the number of descriptor sets want to allocate
    DescriptorUpdateFrequency updateFrequency = DescriptorUpdateFrequency::NONE;
};

struct DescriptorInfo
{
    std::string_view name         = "Untitled";
    DescriptorTypeFlagOneBit type = DescriptorTypeFlagOneBit::UNDEFINED;
    u32 size                      = 0;  //  byte count if root descriptor, otherwise element count of array
    u32 handleIndex               = 0;
    u32 dim               : 4     = 0;
    u32 updateFrequency   : 4     = 0;
    bool isRootDescriptor : 1     = 0;  // whether uniform buffer or root constant or not, which need to be updated per frame
    bool isStaticSampler  : 1     = 0;

    u32 reg               : 14    = 0;  // only used by Vulkan
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
    std::array<Texture*, (u32)TextureDimension::DIM_COUNT> pDefaultTextureSRV{};
    std::array<Texture*, (u32)TextureDimension::DIM_COUNT> pDefaultTextureUAV{};
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
        BlendConstant srcFactor      = BlendConstant::ONE;  // TODO: rename
        BlendConstant dstFactor      = BlendConstant::ZERO;
        BlendConstant srcAlphaFactor = BlendConstant::ONE;
        BlendConstant dstAlphaFactor = BlendConstant::ZERO;
        BlendMode blendMode          = BlendMode::ADD;
        BlendMode blendAlphaMode     = BlendMode::ADD;
        Channel mask                 = Channel::ALL;
    } perRenderTarget[MAX_RENDER_TARGET_ATTACHMENTS];

    u8 attachmentCount                     = 0;                                  /// The render target attachment to set the blend state for.
    BlendStateTargetsFlag renderTargetMask = BlendStateTargetsFlag::TARGET_ALL;  /// Mask that identifies the render targets affected by the blend state.
    bool isIndependentBlend                = false;                              /// Set whether each render target has an unique blend function.
                                                                                 /// When false the blend function in slot 0 will be used for all render targets.
};

// Barrier

struct BarrierInfo
{
    ResourceStateFlags currentState = ResourceStateFlags::UNDEFINED;
    ResourceStateFlags newState     = ResourceStateFlags::UNDEFINED;
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

struct VertexAttrib
{
    ShaderSemantic semantic       = ShaderSemantic::UNDEFINED;
    std::string_view semanticName = "Untitled";
    TinyImageFormat format        = TinyImageFormat_UNDEFINED;
    u32 binding                   = U32_MAX;
    u32 location                  = U32_MAX;
    u32 offset                    = 0;
    VertexAttribRate rate         = VertexAttribRate::VERTEX;
};

struct VertexLayout
{
    u32 attrCount = 0;
    VertexAttrib attrs[MAX_VERTEX_ATTRIBS];
    // u32 strides[MAX_VERTEX_BINDINGS]; // not used yet
};

struct RasterizerStateDesc
{
    CullMode cullMode          = CullMode::BACK;
    i32 depthBias              = 0;
    float slopeScaledDepthBias = 0.0;
    FillMode fillMode          = FillMode::SOLID;
    FrontFace frontFace        = FrontFace::CCW;
    bool enableMultiSample     = false;
    bool enableScissor         = false;
    bool enableDepthClamp      = false;
};

struct DepthStateDesc
{
    bool enableDepthTest         = false;
    bool enableDepthWrite        = false;
    bool enableStencilTest       = false;
    CompareMode depthFunc        = CompareMode::ALWAYS;
    u8 stencilReadMask           = 0xff;
    u8 stencilWriteMask          = 0xff;
    CompareMode stencilFrontFunc = CompareMode::ALWAYS;
    StencilOp stencilFrontFail   = StencilOp::KEEP;
    StencilOp depthFrontFail     = StencilOp::KEEP;
    StencilOp stencilFrontPass   = StencilOp::KEEP;
    CompareMode stencilBackFunc  = CompareMode::ALWAYS;
    StencilOp stencilBackFail    = StencilOp::KEEP;
    StencilOp depthBackFail      = StencilOp::KEEP;
    StencilOp stencilBackPass    = StencilOp::KEEP;
};

struct PipelineCache
{
    nullable<void> pLibrary = nullptr;
    void* pCacheData        = nullptr;
};

struct GraphicsPipelineDesc
{
    Shader* pShaderProgram                = nullptr;
    RootSignature* pRootSignature         = nullptr;
    VertexLayout* pVertexLayout           = nullptr;
    BlendStateDesc* pBlendState           = nullptr;
    DepthStateDesc* pDepthState           = nullptr;
    RasterizerStateDesc* pRasterizerState = nullptr;
    TinyImageFormat* pColorFormats        = nullptr;

    u32 renderTargetCount                 = 0;
    MSAASampleCount sampleCount           = MSAASampleCount::COUNT_1;
    u32 sampleQuality                     = 0;
    TinyImageFormat depthStencilFormat    = TinyImageFormat_UNDEFINED;
    PrimitiveTopology primitiveTopology   = PrimitiveTopology::TRI_LIST;
    bool supportIndirectCommandBuffer     = false;
};

struct ComputePipelineDesc
{
    Shader* pShaderProgram        = nullptr;
    RootSignature* pRootSignature = nullptr;
};

struct PipelineDesc : public DescBase
{
    std::string_view name          = "Untitled";
    PipelineType type              = PipelineType::UNDEFINED;
    nullable<PipelineCache> pCache = nullptr;
    std::pmr::vector<void*> pPipelineExtensions;
    union
    {
        GraphicsPipelineDesc* pGraphicsDesc = nullptr;
        ComputePipelineDesc* pComputeDesc;
    };
};

}  // namespace axe::rhi
