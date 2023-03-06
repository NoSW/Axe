#pragma once
#include "02Rhi/RhiEnums.hpp"
#include <tiny_imageformat/tinyimageformat_query.h>

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
    bool m_canShaderReadFrom[239];
    bool m_canShaderWriteTo[239];
    bool m_canRenderTargetWriteTo[239];
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
class RenderTarget;
class Shader;
class RootSignature;

struct BackendDesc
{
    std::string_view mAppName;
    GpuMode mGpuMode = GPU_MODE_UNLINKED;
};

struct AdapterDesc
{
    bool mSelectedBest = true;
};

struct DeviceDesc
{
    bool mEnableRenderDocLayer      = false;
    bool mRequestAllAvailableQueues = true;
    ShaderModel mShaderModel        = SHADER_MODEL_6_7;
};

struct SemaphoreDesc
{
};

struct FenceDesc
{
    u8 mIsSignaled : 1 = 0;
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

struct CmdPoolDesc
{
    Queue* mpUsedForQueue        = nullptr;
    u8 mShortLived           : 1 = 1;
    u8 mAllowIndividualReset : 1 = 1;
};

struct CmdDesc
{
    CmdPool* mpCmdPool = nullptr;
    u32 mCmdCount      = 1;
    bool mSecondary    = false;
};

struct SamplerDesc
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

struct ShaderStageDesc
{
    std::string_view mEntryPoint = "main";
    std::string_view mFilePath;
    ShaderStage mStage         = SHADER_STAGE_NONE;
    ShaderStageLoadFlag mFlags = SHADER_STAGE_LOAD_FLAG_NONE;
};

struct ShaderConstants  // only supported by Vulkan and Metal APIs
{
    std::pmr::vector<u8> mBlob;
    u32 mIndex;
};

struct ShaderDesc
{
    std::pmr::vector<ShaderStageDesc> mStages;
    std::pmr::vector<ShaderConstants> mConstants;
    ShaderModel mShaderMode = SHADER_MODEL_6_7;
};

struct RootSignatureDesc
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
    u32 mDim             : 4;
    u32 mRootDescriptor  : 1;
    u32 mStaticSampler   : 1;
    u32 mUpdateFrequency : 3;

    u32 mVkStages        : 8;
    u32 mVkType;
    u32 mReg;
};
}  // namespace axe::rhi