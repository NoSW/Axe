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
    std::bitset<TinyImageFormat_Count> mCanShaderReadFrom;
    std::bitset<TinyImageFormat_Count> mCanShaderWriteTo;
    std::bitset<TinyImageFormat_Count> mCanRenderTargetWriteTo;
};

struct GPUVendorPreset
{
    u32 mVendorId                                                          = 0x0;
    u32 mModelId                                                           = 0x0;
    u32 mRevisionId                                                        = 0x0;  // Optional as not all gpu's have that. Default is : 0x00
    char mGpuBackendVersion[CapabilityLevel::MAX_GPU_VENDOR_STRING_LENGTH] = {0};
    char mGpuName[CapabilityLevel::MAX_GPU_VENDOR_STRING_LENGTH]           = {0};
};

struct GPUSettings
{
    u32 mUniformBufferAlignment             = 0;
    u32 mUploadBufferTextureAlignment       = 0;
    u32 mUploadBufferTextureRowAlignment    = 0;
    u32 mMaxVertexInputBindings             = 0;
    u32 mMaxRootSignatureDWORDS             = 0;
    u32 mWaveLaneCount                      = 0;
    WaveOpsSupportFlag mWaveOpsSupportFlags = WAVE_OPS_SUPPORT_FLAG_NONE;
    GPUVendorPreset mGpuVendorPreset;

    // ShadingRate m_shadingRates;  // Variable Rate Shading
    // ShadingRateCaps m_shadingRateCaps;
    u32 mShadingRateTexelWidth       = 0;
    u32 mShadingRateTexelHeight      = 0;
    u32 mTimestampPeriod             = 0;

    u32 mMultiDrawIndirect       : 1 = 0;
    u32 mROVsSupported           : 1 = 0;
    u32 mTessellationSupported   : 1 = 0;
    u32 mGeometryShaderSupported : 1 = 0;
    u32 mGpuBreadcrumbs          : 1 = 0;
    u32 mHDRSupported            : 1 = 0;
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

struct DescBase
{
#if _DEBUG
    std::pmr::string mLabel;
#endif
};

struct BackendDesc : public DescBase
{
    std::string_view mAppName = "Untitled";
};

struct AdapterDesc : public DescBase
{
    bool mSelectedBest = true;
};

struct DeviceDesc : public DescBase
{
    bool mRequestAllAvailableQueues = true;
    ShaderModel mShaderModel        = SHADER_MODEL_6_7;
};

struct SemaphoreDesc : public DescBase
{
};

struct FenceDesc : public DescBase
{
    u8 mIsSignaled : 1 = 0;
};

struct QueueDesc : public DescBase
{
    QueueType mType         = QUEUE_TYPE_GRAPHICS;
    QueueFlag mFlag         = QUEUE_FLAG_NONE;
    QueuePriority mPriority = QUEUE_PRIORITY_NORMAL;
};

struct QueueSubmitDesc : public DescBase
{
    std::pmr::vector<Cmd*> mCmds;
    std::pmr::vector<Semaphore*> mWaitSemaphores;
    std::pmr::vector<Semaphore*> mSignalSemaphores;
    Fence* mpSignalFence = nullptr;
    bool mSubmitDone     = false;
};

struct QueuePresentDesc : public DescBase
{
    std::pmr::vector<Semaphore*> mWaitSemaphores;
    /* TODO: SwapChain */
    void* mpSwapChain = nullptr;
    u8 mIndex         = 0;
    bool mSubmitDone  = false;
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
    window::Window* mpWindow = nullptr;
    Queue* mpPresentQueue;  /// Queues which should be allowed to present
    u32 mImageCount = 0;    /// Number of back buffers in this swapchain
    u32 mWidth      = 0;
    u32 mHeight     = 0;
    ClearValue mColorClearValue;
    bool mUseHDR            = false;  /// Set whether swap chain will be presented using HDR
    bool mEnableVsync       = false;  /// Set whether swap chain will be presented using vsync
    bool mUseFlipSwapEffect = false;  /// We can toggle to using FLIP model if app desires.
};

struct CmdPoolDesc : public DescBase
{
    Queue* mpUsedForQueue        = nullptr;
    u8 mShortLived           : 1 = 1;
    u8 mAllowIndividualReset : 1 = 1;
};

struct CmdDesc : public DescBase
{
    CmdPool* mpCmdPool = nullptr;
    u32 mCmdCount      = 1;
    bool mSecondary    = false;
};

struct SamplerDesc : public DescBase
{
    FilterType mMinFilter    = FILTER_TYPE_LINEAR;
    FilterType mMagFilter    = FILTER_TYPE_LINEAR;
    MipMapMode mMipMapMode   = MIPMAP_MODE_LINEAR;
    AddressMode mAddressU    = ADDRESS_MODE_REPEAT;
    AddressMode mAddressV    = ADDRESS_MODE_REPEAT;
    AddressMode mAddressW    = ADDRESS_MODE_REPEAT;
    float mMipLodBias        = 0.0f;
    float mMinLod            = 0.0f;
    float mMaxLod            = 0.0f;
    float mMaxAnisotropy     = 0.0f;
    bool mSetLodRange        = false;
    CompareMode mCompareFunc = CMP_MODE_NEVER;

    struct
    {
        TinyImageFormat mFormat           = TinyImageFormat_UNDEFINED;
        SamplerModelConversion mModel     = SAMPLER_MODEL_CONVERSION_RGB_IDENTITY;
        SamplerRange mRange               = SAMPLER_RANGE_FULL;
        SampleLocation mChromaOffsetX     = SAMPLE_LOCATION_COSITED;
        SampleLocation mChromaOffsetY     = SAMPLE_LOCATION_COSITED;
        FilterType mChromaFilter          = FILTER_TYPE_LINEAR;
        bool mForceExplicitReconstruction = false;
    } mSamplerConversionDesc;
};

struct TextureDesc : public DescBase
{
    const void* pNativeHandle = nullptr;     // Pointer to native texture handle if the texture does not own underlying resource
    const char* pName         = "Untitled";  // Debug name used in gpu profile
    ClearValue mClearValue{};                // Optimized clear value (recommended to use this same value when clearing the render target)
    // #if defined(VULKAN)
    //     VkSamplerYcbcrConversionInfo* pVkSamplerYcbcrConversionInfo;
    // #endif

    TextureCreationFlags mFlags  = TEXTURE_CREATION_FLAG_NONE;  // decides memory allocation strategy, sharing access,...
    u32 mWidth                   = 0;
    u32 mHeight                  = 0;
    u32 mDepth                   = 0;  // Should be 1 if not a mType is not TEXTURE_TYPE_3D)
    u32 mArraySize               = 0;  // Should be 1 if texture is not a texture array or cubemap
    u32 mMipLevels               = 0;  // number of mipmap levels
    MSAASampleCount mSampleCount = MSAA_SAMPLE_COUNT_1;
    u32 mSampleQuality           = 0;  // The valid range is between zero and the value appropriate for mSampleCount
    TinyImageFormat mFormat      = TinyImageFormat_UNDEFINED;
    ResourceState mStartState    = RESOURCE_STATE_UNDEFINED;   // What state will the texture get created in
    DescriptorType mDescriptors  = DESCRIPTOR_TYPE_UNDEFINED;  // Descriptor creation
};

struct BufferDesc : public DescBase
{
    /// Size of the buffer (in bytes)
    u64 mSize                         = 0;
    /// Set this to specify a counter buffer for this buffer (applicable to BUFFER_USAGE_STORAGE_SRV, BUFFER_USAGE_STORAGE_UAV)
    Buffer* mpCounterBuffer           = nullptr;
    /// Index of the first element accessible by the SRV/UAV (applicable to BUFFER_USAGE_STORAGE_SRV, BUFFER_USAGE_STORAGE_UAV)
    u64 mFirstElement                 = 0;
    /// Number of elements in the buffer (applicable to BUFFER_USAGE_STORAGE_SRV, BUFFER_USAGE_STORAGE_UAV)
    u64 mElementCount                 = 0;
    /// Size of each element (in bytes) in the buffer (applicable to BUFFER_USAGE_STORAGE_SRV, BUFFER_USAGE_STORAGE_UAV)
    u64 mStructStride                 = 0;
    /// Debug name used in gpu profile
    std::string_view mName            = "Untitled";
    /// Alignment
    u32 mAlignment                    = 0;
    /// Decides which memory heap buffer will use (default, upload, readback)
    ResourceMemoryUsage mMemoryUsage  = RESOURCE_MEMORY_USAGE_UNKNOWN;
    /// Creation flags of the buffer
    BufferCreationFlags mFlags        = BUFFER_CREATION_FLAG_NONE;
    /// What type of queue the buffer is owned by
    QueueType mQueueType              = QUEUE_TYPE_MAX;
    /// What state will the buffer get created in
    ResourceState mStartState         = RESOURCE_STATE_UNDEFINED;
    /// ICB draw type
    IndirectArgumentType mICBDrawType = INDIRECT_ARG_INVALID;
    /// ICB max vertex buffers slots count
    u32 mICBMaxCommandCount           = 0;
    /// Format of the buffer (applicable to typed storage buffers (Buffer<T>)
    TinyImageFormat mFormat           = TinyImageFormat_UNDEFINED;
    /// Flags specifying the suitable usage of this buffer (Uniform buffer, Vertex Buffer, Index Buffer,...)
    DescriptorType mDescriptors       = DESCRIPTOR_TYPE_UNDEFINED;
};

struct RenderTargetDesc : public DescBase
{
    TextureCreationFlags mFlags      = TEXTURE_CREATION_FLAG_NONE;  // decides memory allocation strategy, sharing access,...
    u32 mWidth                       = 0;
    u32 mHeight                      = 0;
    u32 mDepth                       = 1;  // (Should be 1 if not a mType is not TEXTURE_TYPE_3D)
    u32 mArraySize                   = 1;  // Texture array size (Should be 1 if texture is not a texture array or cubemap)
    u32 mMipLevels                   = 0;  // Number of mip levels
    MSAASampleCount mMSAASampleCount = MSAA_SAMPLE_COUNT_1;
    TinyImageFormat mFormat          = TinyImageFormat_UNDEFINED;  // Internal image format
    ResourceState mStartState        = RESOURCE_STATE_UNDEFINED;   // What flag will the texture get created in
    ClearValue mClearValue{.rgba = {0, 0, 0, 0}};                  // Optimized clear value (recommended to use this same value when clearing the RenderTarget)
    u32 mSampleQuality          = 1;                               // The image quality level. The higher the quality, the lower the performance. The valid range is between zero and the value appropriate for mSampleCount
    DescriptorType mDescriptors = DESCRIPTOR_TYPE_UNDEFINED;       // Descriptor creation
    const void* mpNativeHandle  = nullptr;
    const char* mpName          = "Untitled";  // Debug name used in gpu profile
};

struct ShaderStageDesc
{
    std::string_view mEntryPoint = "main";
    std::string_view mFilePath;
    ShaderStageFlag mStage     = SHADER_STAGE_FLAG_NONE;
    ShaderStageLoadFlag mFlags = SHADER_STAGE_LOAD_FLAG_NONE;
};

struct ShaderConstants  // only supported by Vulkan APIs
{
    std::pmr::vector<u8> mBlob;
    u32 mIndex;
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
    std::pmr::vector<std::string_view> mStaticSamplerNames;
    std::pmr::vector<Sampler*> mStaticSamplers;
    u32 mMaxBindlessTextures  = 0;
    RootSignatureFlags mFlags = ROOT_SIGNATURE_FLAG_NONE;
};

struct DescriptorInfo
{
    std::string_view mName;
    u32 mType;
    u32 mSize;
    u32 mHandleIndex;
    u32 mDim              : 4;
    u32 mIsRootDescriptor : 1;
    u32 mStaticSampler    : 1;
    u32 mUpdateFrequency  : 3;

    u32 mVkStages         : 8;
    u32 mVkType;
    u32 mReg;
};

struct NullDescriptors
{
    std::array<Texture*, TEXTURE_DIM_COUNT> mpDefaultTextureSRV{};
    std::array<Texture*, TEXTURE_DIM_COUNT> mpDefaultTextureUAV{};
    Buffer* mpDefaultBufferSRV          = nullptr;
    Buffer* mpDefaultBufferUAV          = nullptr;
    Sampler* mpDefaultSampler           = nullptr;
    // Mutex mSubmitMutex;

    // #TODO - Remove after we have a better way to specify initial resource state
    // Unlike DX12, Vulkan textures start in undefined layout.
    // With this, we transition them to the specified layout so app code doesn't have to worry about this
    // Mutex mInitialTransitionMutex;
    Queue* mpInitialTransitionQueue     = nullptr;
    CmdPool* mpInitialTransitionCmdPool = nullptr;
    Cmd* mpInitialTransitionCmd         = nullptr;
    Fence* mpInitialTransitionFence     = nullptr;
};

// Raster
struct BlendStateDesc
{
    struct
    {
        BlendConstant mSrcFactor      = BC_ONE;
        BlendConstant mDstFactor      = BC_ZERO;
        BlendConstant mSrcAlphaFactor = BC_ONE;
        BlendConstant mDstAlphaFactor = BC_ZERO;
        BlendMode mBlendMode          = BM_ADD;
        BlendMode mBlendAlphaMode     = BM_ADD;
        i32 mMask                     = CH_ALL;
    } mPerRenderTarget[MAX_RENDER_TARGET_ATTACHMENTS];

    u8 mAttachmentCount                 = 0;                       /// The render target attachment to set the blend state for.
    BlendStateTargets mRenderTargetMask = BLEND_STATE_TARGET_ALL;  /// Mask that identifies the render targets affected by the blend state.
    bool mAlphaToCoverage               = false;                   /// Set whether alpha to coverage should be enabled.
    bool mIndependentBlend              = false;                   /// Set whether each render target has an unique blend function.
                                                                   /// When false the blend function in slot 0 will be used for all render targets.
};

// Barrier

struct BarrierInfo
{
    ResourceState mCurrentState = RESOURCE_STATE_UNDEFINED;
    ResourceState mNewState     = RESOURCE_STATE_UNDEFINED;
    u8 mBeginOnly : 1           = 0;
    u8 mEndOnly   : 1           = 0;
    u8 mAcquire   : 1           = 0;
    u8 mRelease   : 1           = 0;
    u8 mQueueType : 5           = 0;
};

struct ImageBarrier
{
    BarrierInfo mBarrierInfo{};
    u8 mSubresourceBarrier : 1 = 0;  // Specify whether following barrier targets particular subresource
    u8 mMipLevel           : 7 = 0;  // ignored if mSubresourceBarrier is false
    u16 mArrayLayer            = 0;  // ignored if mSubresourceBarrier is false
};

struct TextureBarrier
{
    ImageBarrier mImageBarrier{};
    Texture* mpTexture = nullptr;
};

struct RenderTargetBarrier
{
    ImageBarrier mImageBarrier{};
    RenderTarget* mpRenderTarget = nullptr;
};

struct BufferBarrier
{
    BarrierInfo mBarrierInfo{};
    Buffer* pBuffer = nullptr;
};

}  // namespace axe::rhi
