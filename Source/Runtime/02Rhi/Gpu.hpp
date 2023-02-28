#pragma once

#include <02Rhi/Config.hpp>

namespace axe::rhi
{

//////////////////////////////////////////////////////////////////////////////////////////////
//                            enum
//////////////////////////////////////////////////////////////////////////////////////////////
enum GpuMode
{
    GPU_MODE_SINGLE = 0,
    GPU_MODE_LINKED,
    GPU_MODE_UNLINKED,
};

enum WaveOpsSupportFlag
{
    WAVE_OPS_SUPPORT_FLAG_NONE                 = 0x0,
    WAVE_OPS_SUPPORT_FLAG_BASIC_BIT            = 0x00000001,
    WAVE_OPS_SUPPORT_FLAG_VOTE_BIT             = 0x00000002,
    WAVE_OPS_SUPPORT_FLAG_ARITHMETIC_BIT       = 0x00000004,
    WAVE_OPS_SUPPORT_FLAG_BALLOT_BIT           = 0x00000008,
    WAVE_OPS_SUPPORT_FLAG_SHUFFLE_BIT          = 0x00000010,
    WAVE_OPS_SUPPORT_FLAG_SHUFFLE_RELATIVE_BIT = 0x00000020,
    WAVE_OPS_SUPPORT_FLAG_CLUSTERED_BIT        = 0x00000040,
    WAVE_OPS_SUPPORT_FLAG_QUAD_BIT             = 0x00000080,
    WAVE_OPS_SUPPORT_FLAG_PARTITIONED_BIT_NV   = 0x00000100,
    WAVE_OPS_SUPPORT_FLAG_ALL                  = 0x7FFFFFFF
};

enum GpuVender
{
    GPU_VENDOR_ID_NVIDIA  = 0x10DE,
    GPU_VENDOR_ID_AMD     = 0x1002,
    GPU_VENDOR_ID_AMD_1   = 0x1022,
    GPU_VENDOR_ID_INTEL   = 0x163C,
    GPU_VENDOR_ID_INTEL_1 = 0x8086,
    GPU_VENDOR_ID_INTEL_2 = 0x8087
};

// default capability levels of the driver
enum CapabilityLevel
{
    MAX_INSTANCE_EXTENSIONS       = 64,
    MAX_DEVICE_EXTENSIONS         = 64,
    MAX_LINKED_GPUS               = 4,  // Max number of GPUs in SLI or Cross-Fire
    MAX_UNLINKED_GPUS             = 4,  // Max number of GPUs in unlinked mode
    MAX_MULTIPLE_GPUS             = 4,  // Max number of GPus for either linked or unlinked mode.
    MAX_RENDER_TARGET_ATTACHMENTS = 8,
    MAX_VERTEX_BINDINGS           = 15,
    MAX_VERTEX_ATTRIBS            = 15,
    MAX_RESOURCE_NAME_LENGTH      = 256,
    MAX_SEMANTIC_NAME_LENGTH      = 128,
    MAX_DEBUG_NAME_LENGTH         = 128,
    MAX_MIP_LEVELS                = 0xFFFFFFFF,
    MAX_SWAPCHAIN_IMAGES          = 3,
    MAX_GPU_VENDOR_STRING_LENGTH  = 256,  // max size for GPUVendorPreset strings

    // only for vulkan
    MAX_PLANE_COUNT               = 3,

    //  MAX_DESCRIPTOR_POOL_SIZE_ARRAY_COUNT = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT + 1,  // only for vulkan
};

//////////////////////////////////////////////////////////////////////////////////////////////
//                             struct
//////////////////////////////////////////////////////////////////////////////////////////////
struct GPUCapBits
{
    bool m_canShaderReadFrom[239];
    bool m_canShaderWriteTo[239];
    bool m_canRenderTargetWriteTo[239];
};

struct GPUVendorPreset
{
    u32 mVendorId                                                         = 0x0;
    u32 mModelId                                                          = 0x0;
    u32 mRevisionId                                                       = 0x0;  // Optional as not all gpu's have that. Default is : 0x00
    char mGpuDriverVersion[CapabilityLevel::MAX_GPU_VENDOR_STRING_LENGTH] = {0};
    char mGpuName[CapabilityLevel::MAX_GPU_VENDOR_STRING_LENGTH]          = {0};
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

    u32 mMultiDrawIndirect       : 1 = 0;
    u32 mROVsSupported           : 1 = 0;
    u32 mTessellationSupported   : 1 = 0;
    u32 mGeometryShaderSupported : 1 = 0;
    u32 mGpuBreadcrumbs          : 1 = 0;
    u32 mHDRSupported            : 1 = 0;
};

}  // namespace axe::rhi