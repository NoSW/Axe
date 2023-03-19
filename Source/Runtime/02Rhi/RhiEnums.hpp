#pragma once
#include "02Rhi/Config.hpp"

#include "00Core/Log/Log.hpp"
#include "00Core/Reflection/Reflection.hpp"

#include <tiny_imageformat/tinyimageformat_base.h>

#include <string_view>
#include <unordered_map>

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
    WAVE_OPS_SUPPORT_FLAG_NONE                 = 0,
    WAVE_OPS_SUPPORT_FLAG_BASIC_BIT            = 1 << 0,
    WAVE_OPS_SUPPORT_FLAG_VOTE_BIT             = 1 << 1,
    WAVE_OPS_SUPPORT_FLAG_ARITHMETIC_BIT       = 1 << 2,
    WAVE_OPS_SUPPORT_FLAG_BALLOT_BIT           = 1 << 3,
    WAVE_OPS_SUPPORT_FLAG_SHUFFLE_BIT          = 1 << 4,
    WAVE_OPS_SUPPORT_FLAG_SHUFFLE_RELATIVE_BIT = 1 << 5,
    WAVE_OPS_SUPPORT_FLAG_CLUSTERED_BIT        = 1 << 6,
    WAVE_OPS_SUPPORT_FLAG_QUAD_BIT             = 1 << 7,
    WAVE_OPS_SUPPORT_FLAG_PARTITIONED_BIT_NV   = 1 << 8,
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

// default capability levels of the backend
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
};

enum FenceStatus
{
    FENCE_STATUS_COMPLETE = 0,
    FENCE_STATUS_INCOMPLETE,
    FENCE_STATUS_NOTSUBMITTED,
};

enum QueueType
{
    QUEUE_TYPE_GRAPHICS = 0,
    QUEUE_TYPE_COMPUTE,
    QUEUE_TYPE_TRANSFER,
    QUEUE_TYPE_COUNT,
    QUEUE_TYPE_MAX = QUEUE_TYPE_COUNT
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

enum FilterType
{
    FILTER_TYPE_NEAREST = 0,
    FILTER_TYPE_LINEAR  = 1,
};

enum MipMapMode
{
    MIPMAP_MODE_NEAREST = 0,
    MIPMAP_MODE_LINEAR  = 1,
};

enum AddressMode
{
    ADDRESS_MODE_MIRROR          = 0,
    ADDRESS_MODE_REPEAT          = 1,
    ADDRESS_MODE_CLAMP_TO_EDGE   = 2,
    ADDRESS_MODE_CLAMP_TO_BORDER = 3,
};

enum CompareMode
{
    CMP_MODE_NEVER    = 0,
    CMP_MODE_LESS     = 1,
    CMP_MODE_EQUAL    = 2,
    CMP_MODE_LEQUAL   = 3,
    CMP_MODE_GREATER  = 4,
    CMP_MODE_NOTEQUAL = 5,
    CMP_MODE_GEQUAL   = 6,
    CMP_MODE_ALWAYS   = 7,
    CMP_MODE_COUNT    = 8
};

enum SamplerRange
{
    SAMPLER_RANGE_FULL   = 0,
    SAMPLER_RANGE_NARROW = 1,
};

enum SampleLocation
{
    SAMPLE_LOCATION_COSITED  = 0,
    SAMPLE_LOCATION_MIDPOINT = 1,
};

enum SamplerModelConversion
{
    SAMPLER_MODEL_CONVERSION_RGB_IDENTITY   = 0,
    SAMPLER_MODEL_CONVERSION_YCBCR_IDENTITY = 1,
    SAMPLER_MODEL_CONVERSION_YCBCR_709      = 2,
    SAMPLER_MODEL_CONVERSION_YCBCR_601      = 3,
    SAMPLER_MODEL_CONVERSION_YCBCR_2020     = 4,
};

enum ResourceState
{
    RESOURCE_STATE_UNDEFINED                         = 0,
    RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER        = 1 << 0,
    RESOURCE_STATE_INDEX_BUFFER                      = 1 << 1,
    RESOURCE_STATE_RENDER_TARGET                     = 1 << 2,
    RESOURCE_STATE_UNORDERED_ACCESS                  = 1 << 3,
    RESOURCE_STATE_DEPTH_WRITE                       = 1 << 4,
    RESOURCE_STATE_DEPTH_READ                        = 1 << 5,
    RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE         = 1 << 6,
    RESOURCE_STATE_PIXEL_SHADER_RESOURCE             = 1 << 7,
    RESOURCE_STATE_STREAM_OUT                        = 1 << 8,
    RESOURCE_STATE_INDIRECT_ARGUMENT                 = 1 << 9,
    RESOURCE_STATE_COPY_DEST                         = 1 << 10,
    RESOURCE_STATE_COPY_SOURCE                       = 1 << 11,
    RESOURCE_STATE_PRESENT                           = 1 << 12,
    RESOURCE_STATE_COMMON                            = 1 << 13,
    RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE = 1 << 14,
    RESOURCE_STATE_SHADING_RATE_SOURCE               = 1 << 15,
    RESOURCE_STATE_SHADER_RESOURCE                   = RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
    RESOURCE_STATE_GENERIC_READ                      = RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | RESOURCE_STATE_INDEX_BUFFER | RESOURCE_STATE_RENDER_TARGET |
                                  RESOURCE_STATE_PIXEL_SHADER_RESOURCE | RESOURCE_STATE_INDIRECT_ARGUMENT | RESOURCE_STATE_COPY_SOURCE,
};

enum TextureCreationFlags
{
    // Default flag (Texture will use default allocation strategy decided by the api specific allocator)
    TEXTURE_CREATION_FLAG_NONE                      = 0,
    // Texture will allocate its own memory (COMMITTED resource)
    TEXTURE_CREATION_FLAG_OWN_MEMORY_BIT            = 1 << 0,
    // Texture will be allocated in memory which can be shared among multiple processes
    TEXTURE_CREATION_FLAG_EXPORT_BIT                = 1 << 1,
    // Texture will be allocated in memory which can be shared among multiple gpus
    TEXTURE_CREATION_FLAG_EXPORT_ADAPTER_BIT        = 1 << 2,
    // Texture will be imported from a handle created in another process
    TEXTURE_CREATION_FLAG_IMPORT_BIT                = 1 << 3,
    // Use ESRAM to store this texture
    TEXTURE_CREATION_FLAG_ESRAM                     = 1 << 4,
    // Use on-tile memory to store this texture
    TEXTURE_CREATION_FLAG_ON_TILE                   = 1 << 5,
    // Prevent compression meta data from generating (XBox)
    TEXTURE_CREATION_FLAG_NO_COMPRESSION            = 1 << 6,
    // Force 2D instead of automatically determining dimension based on width, height, depth
    TEXTURE_CREATION_FLAG_FORCE_2D                  = 1 << 7,
    // Force 3D instead of automatically determining dimension based on width, height, depth
    TEXTURE_CREATION_FLAG_FORCE_3D                  = 1 << 8,
    // Display target
    TEXTURE_CREATION_FLAG_ALLOW_DISPLAY_TARGET      = 1 << 9,
    // Create an sRGB texture.
    TEXTURE_CREATION_FLAG_SRGB                      = 1 << 10,
    // Create a normal map texture
    TEXTURE_CREATION_FLAG_NORMAL_MAP                = 1 << 11,
    // Fast clear
    TEXTURE_CREATION_FLAG_FAST_CLEAR                = 1 << 12,
    // Fragment mask
    TEXTURE_CREATION_FLAG_FRAG_MASK                 = 1 << 13,
    // Doubles the amount of array layers of the texture when rendering VR. Also forces the texture to be a 2D Array texture.
    TEXTURE_CREATION_FLAG_VR_MULTIVIEW              = 1 << 14,
    // Binds the FFR fragment density if this texture is used as a render target.
    TEXTURE_CREATION_FLAG_VR_FOVEATED_RENDERING     = 1 << 15,
    // Creates resolve attachment for auto resolve (MSAA on tiled architecture - Resolve can be done on tile through render pass)
    TEXTURE_CREATION_FLAG_CREATE_RESOLVE_ATTACHMENT = 1 << 16,
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

enum ResourceFlag
{
    RESOURCE_FLAG_UNDEFINED                         = 0,
    RESOURCE_FLAG_VERTEX_AND_CONSTANT_BUFFER        = 1 << 0,
    RESOURCE_FLAG_INDEX_BUFFER                      = 1 << 1,
    RESOURCE_FLAG_RENDER_TARGET                     = 1 << 2,
    RESOURCE_FLAG_UNORDERED_ACCESS                  = 1 << 3,
    RESOURCE_FLAG_DEPTH_WRITE                       = 1 << 4,
    RESOURCE_FLAG_DEPTH_READ                        = 1 << 5,
    RESOURCE_FLAG_NON_PIXEL_SHADER_RESOURCE         = 1 << 6,
    RESOURCE_FLAG_PIXEL_SHADER_RESOURCE             = 1 << 7,
    RESOURCE_FLAG_STREAM_OUT                        = 1 << 8,
    RESOURCE_FLAG_INDIRECT_ARGUMENT                 = 1 << 9,
    RESOURCE_FLAG_COPY_DEST                         = 1 << 10,
    RESOURCE_FLAG_COPY_SOURCE                       = 1 << 11,
    RESOURCE_FLAG_PRESENT                           = 1 << 12,
    RESOURCE_FLAG_COMMON                            = 1 << 13,
    RESOURCE_FLAG_RAYTRACING_ACCELERATION_STRUCTURE = 1 << 14,
    RESOURCE_FLAG_SHADING_RATE_SOURCE               = 1 << 15,
    RESOURCE_FLAG_SHADER_RESOURCE                   = RESOURCE_FLAG_NON_PIXEL_SHADER_RESOURCE | RESOURCE_FLAG_PIXEL_SHADER_RESOURCE,
    RESOURCE_FLAG_GENERIC_READ                      = RESOURCE_FLAG_VERTEX_AND_CONSTANT_BUFFER | RESOURCE_FLAG_INDEX_BUFFER |
                                 RESOURCE_FLAG_NON_PIXEL_SHADER_RESOURCE | RESOURCE_FLAG_PIXEL_SHADER_RESOURCE |
                                 RESOURCE_FLAG_INDIRECT_ARGUMENT | RESOURCE_FLAG_COPY_SOURCE,
};

enum ResourceMemoryUsage
{

    RESOURCE_MEMORY_USAGE_UNKNOWN    = 0,  // No intended memory usage specified.
    RESOURCE_MEMORY_USAGE_GPU_ONLY   = 1,  // Memory will be used on device only, no need to be mapped on host.
    RESOURCE_MEMORY_USAGE_CPU_ONLY   = 2,  // Memory will be mapped on host. Could be used for transfer to device.
    RESOURCE_MEMORY_USAGE_CPU_TO_GPU = 3,  // Memory will be used for frequent (dynamic) updates from host and reads on device.
    RESOURCE_MEMORY_USAGE_GPU_TO_CPU = 4,  // Memory will be used for writing on device and readback on host.
    RESOURCE_MEMORY_USAGE_COUNT,
    RESOURCE_MEMORY_USAGE_MAX_ENUM = 0x7FFFFFFF
};

enum BufferCreationFlags
{
    BUFFER_CREATION_FLAG_NONE                        = 1 << 0,  // Default flag (Buffer will use aliased memory, buffer will not be cpu accessible until mapBuffer is called)
    BUFFER_CREATION_FLAG_OWN_MEMORY_BIT              = 1 << 2,  // Buffer will allocate its own memory (COMMITTED resource)
    BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT          = 1 << 3,  // Buffer will be persistently mapped
    BUFFER_CREATION_FLAG_ESRAM                       = 1 << 4,  // Use ESRAM to store this buffer
    BUFFER_CREATION_FLAG_NO_DESCRIPTOR_VIEW_CREATION = 1 << 5,  // Flag to specify not to allocate descriptors for the resource
    BUFFER_CREATION_FLAG_HOST_VISIBLE_VKONLY         = 1 << 6,  // Memory Host Flags
    BUFFER_CREATION_FLAG_HOST_COHERENT_VKONLY        = 1 << 7,  // Memory Host Flags

};

enum TextureDimension
{
    TEXTURE_DIM_1D,
    TEXTURE_DIM_2D,
    TEXTURE_DIM_2DMS,
    TEXTURE_DIM_3D,
    TEXTURE_DIM_CUBE,
    TEXTURE_DIM_1D_ARRAY,
    TEXTURE_DIM_2D_ARRAY,
    TEXTURE_DIM_2DMS_ARRAY,
    TEXTURE_DIM_CUBE_ARRAY,
    TEXTURE_DIM_COUNT,
    TEXTURE_DIM_UNDEFINED,

};

enum IndirectArgumentType
{
    INDIRECT_ARG_INVALID,
    INDIRECT_DRAW,
    INDIRECT_DRAW_INDEX,
    INDIRECT_DISPATCH,
    INDIRECT_VERTEX_BUFFER,
    INDIRECT_INDEX_BUFFER,
    INDIRECT_CONSTANT,
    INDIRECT_CONSTANT_BUFFER_VIEW_DXONLY,    // only for dx
    INDIRECT_SHADER_RESOURCE_VIEW_DXONLY,    // only for dx
    INDIRECT_UNORDERED_ACCESS_VIEW_DXONLY,   // only for dx
    INDIRECT_COMMAND_BUFFER_MTONLY,          // metal ICB
    INDIRECT_COMMAND_BUFFER_RESET_MTONLY,    // metal ICB reset
    INDIRECT_COMMAND_BUFFER_OPTIMIZE_MTONLY  // metal ICB optimization
};

enum DescriptorType
{
    DESCRIPTOR_TYPE_UNDEFINED                                 = 0,
    DESCRIPTOR_TYPE_SAMPLER                                   = 1 << 0,
    DESCRIPTOR_TYPE_TEXTURE                                   = 1 << 1,                                // SRV Read only texture
    DESCRIPTOR_TYPE_RW_TEXTURE                                = 1 << 2,                                /// UAV Texture
    DESCRIPTOR_TYPE_BUFFER                                    = 1 << 3,                                // SRV Read only buffer
    DESCRIPTOR_TYPE_BUFFER_RAW                                = (1 << 4) | DESCRIPTOR_TYPE_BUFFER,     // SRV Read only buffer
    DESCRIPTOR_TYPE_RW_BUFFER                                 = 1 << 5,                                /// UAV Buffer
    DESCRIPTOR_TYPE_RW_BUFFER_RAW                             = (1 << 6) | DESCRIPTOR_TYPE_RW_BUFFER,  /// UAV Buffer
    DESCRIPTOR_TYPE_UNIFORM_BUFFER                            = 1 << 7,                                /// Uniform buffer
    DESCRIPTOR_TYPE_ROOT_CONSTANT                             = 1 << 8,                                /// Push constant / Root constant

    /// IA
    DESCRIPTOR_TYPE_VERTEX_BUFFER                             = 1 << 9,
    DESCRIPTOR_TYPE_INDEX_BUFFER                              = 1 << 10,
    DESCRIPTOR_TYPE_INDIRECT_BUFFER                           = 1 << 11,

    DESCRIPTOR_TYPE_TEXTURE_CUBE                              = 1 << 12 | DESCRIPTOR_TYPE_TEXTURE,  /// Cubemap SRV
    DESCRIPTOR_TYPE_RENDER_TARGET_MIP_SLICES                  = 1 << 13,                            /// RTV / DSV per mip slice
    DESCRIPTOR_TYPE_RENDER_TARGET_ARRAY_SLICES                = 1 << 14,                            /// RTV / DSV per array slice

    /// RTV / DSV per depth slice
    DESCRIPTOR_TYPE_RENDER_TARGET_DEPTH_SLICES                = 1 << 15,
    DESCRIPTOR_TYPE_RAY_TRACING                               = 1 << 16,
    DESCRIPTOR_TYPE_INDIRECT_COMMAND_BUFFER                   = 1 << 17,

    /// Subpass input (only available in Vulkan)
    DESCRIPTOR_TYPE_INPUT_ATTACHMENT_VKONLY                   = 1 << 18,
    DESCRIPTOR_TYPE_TEXEL_BUFFER_VKONLY                       = 1 << 19,
    DESCRIPTOR_TYPE_RW_TEXEL_BUFFER_VKONLY                    = 1 << 20,
    DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER_VKONLY             = 1 << 21,

    /// Khronos extension ray tracing (only available in Vulkan))
    DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_VKONLY             = 1 << 22,
    DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_BUILD_INPUT_VKONLY = 1 << 23,
    DESCRIPTOR_TYPE_SHADER_DEVICE_ADDRESS_VKONLY              = 1 << 24,
    DESCRIPTOR_TYPE_SHADER_BINDING_TABLE_VKONLY               = 1 << 25,
};

enum ShaderStageFlag
{
    SHADER_STAGE_FLAG_NONE       = 0,
    SHADER_STAGE_FLAG_VERT       = 1 << 0,
    SHADER_STAGE_FLAG_TESC       = 1 << 1,
    SHADER_STAGE_FLAG_TESE       = 1 << 2,
    SHADER_STAGE_FLAG_GEOM       = 1 << 3,
    SHADER_STAGE_FLAG_FRAG       = 1 << 4,
    SHADER_STAGE_FLAG_COMP       = 1 << 5,
    SHADER_STAGE_FLAG_RAYTRACING = 1 << 6,
    SHADER_STAGE_FLAG_MAX        = SHADER_STAGE_FLAG_RAYTRACING + 1,
    SHADER_STAGE_FLAG_COUNT      = 7,

    SHADER_STAGE_FLAG_HULL       = SHADER_STAGE_FLAG_TESC,
    SHADER_STAGE_FLAG_DOMAIN     = SHADER_STAGE_FLAG_TESE,

    SHADER_STAGE_FLAG_GRAPHICS   = SHADER_STAGE_FLAG_VERT | SHADER_STAGE_FLAG_TESC | SHADER_STAGE_FLAG_TESE | SHADER_STAGE_FLAG_GEOM | SHADER_STAGE_FLAG_FRAG,
};

enum ShaderModel
{
    // (From) 5.1 is supported on all DX12 hardware
    SHADER_MODEL_5_1 = 0x51,
    SHADER_MODEL_6_0 = 0x60,
    SHADER_MODEL_6_1 = 0x61,
    SHADER_MODEL_6_2 = 0x62,
    SHADER_MODEL_6_3 = 0x63,
    SHADER_MODEL_6_4 = 0x64,
    SHADER_MODEL_6_5 = 0x65,
    SHADER_MODEL_6_6 = 0x66,
    SHADER_MODEL_6_7 = 0x67,
    SHADER_MODEL_HIGHEST
};

enum ShaderStageLoadFlag
{
    SHADER_STAGE_LOAD_FLAG_NONE,
    SHADER_STAGE_LOAD_FLAG_ENABLE_PS_PRIMITIVEID,
};

enum PipelineType
{
    PIPELINE_TYPE_UNDEFINED = 0,
    PIPELINE_TYPE_COMPUTE,
    PIPELINE_TYPE_GRAPHICS,
    PIPELINE_TYPE_RAYTRACING,
    PIPELINE_TYPE_COUNT,
};

enum DescriptorUpdateFrequency
{
    DESCRIPTOR_UPDATE_FREQ_NONE = 0,
    DESCRIPTOR_UPDATE_FREQ_PER_FRAME,
    DESCRIPTOR_UPDATE_FREQ_PER_BATCH,
    DESCRIPTOR_UPDATE_FREQ_PER_DRAW,
    DESCRIPTOR_UPDATE_FREQ_COUNT,
};

enum RootSignatureFlags
{
    ROOT_SIGNATURE_FLAG_NONE,       // Default flag
    ROOT_SIGNATURE_FLAG_LOCAL_BIT,  // used mainly in raytracing shaders
};

enum AdapterType
{
    ADAPTER_TYPE_OTHER          = 0,
    ADAPTER_TYPE_INTEGRATED_GPU = 1,
    ADAPTER_TYPE_DISCRETE_GPU   = 2,
    ADAPTER_TYPE_VIRTUAL_GPU    = 3,
    ADAPTER_TYPE_CPU            = 4,
    ADAPTER_TYPE_COUNT
};

enum GraphicsApi
{
    GRAPHICS_API_NULL      = AXE_02RHI_API_FLAG_NULL,
    GRAPHICS_API_VULKAN    = AXE_02RHI_API_FLAG_VULKAN,
    GRAPHICS_API_D3D12     = AXE_02RHI_API_FLAG_D3D12,
    GRAPHICS_API_METAL     = AXE_02RHI_API_FLAG_METAL,
    GRAPHICS_API_AVAILABLE = AXE_02RHI_API_FLAG_AVAILABLE,
};

inline constexpr ShaderStageFlag get_shader_stage(std::string_view ext)
{
    if (ext == ".vert") { return SHADER_STAGE_FLAG_VERT; }
    else if (ext == ".tesc") { return SHADER_STAGE_FLAG_TESC; }
    else if (ext == ".tese") { return SHADER_STAGE_FLAG_TESE; }
    else if (ext == ".geom") { return SHADER_STAGE_FLAG_GEOM; }
    else if (ext == ".frag") { return SHADER_STAGE_FLAG_FRAG; }
    else if (ext == ".comp") { return SHADER_STAGE_FLAG_COMP; }
    else if (ext == ".rgen" || ext == ".rmiss" || ext == ".rchit" ||
             ext == ".rint" || ext == ".rahit" || ext == "rcall") { return SHADER_STAGE_FLAG_RAYTRACING; }
    else { return SHADER_STAGE_FLAG_NONE; }
};

struct VertexInput
{
    // resource name
    std::string_view mName;

    // The size of the attribute
    u32 mSize;
};

struct ShaderResource
{
    std::string_view mName;
    ShaderStageFlag mUsedShaderStage;
    TextureDimension mDim;
    DescriptorType mType;
    u32 mSet;
    u32 mBindingLocation;
    u32 mSize;

    bool operator==(const ShaderResource& b) const
    {
        return mType == b.mType && mSet == b.mSet && mBindingLocation == b.mBindingLocation && mName == b.mName;
    }

    bool operator!=(const ShaderResource& b) const { return !(*this == b); }
};

struct ShaderVariable
{
    std::string_view mName;
    u32 mParentIndex;
    u32 mOffset;
    u32 mSize;
    bool operator==(const ShaderVariable& b) const
    {
        return mOffset == b.mOffset && mSize == b.mSize && mName == b.mName;
    }

    bool operator!=(const ShaderVariable& b) const { return !(*this == b); }
};

struct ShaderReflection
{
    std::pmr::vector<VertexInput> mVertexInputs;
    std::pmr::vector<ShaderResource> mShaderResources;
    std::pmr::vector<ShaderVariable> mShaderVariables;
    std::string_view mEntryPoint_VkOnly;
    ShaderStageFlag mShaderStage;
    u32 mNumThreadsPerGroup[3];  // for compute shader
    u32 mNumControlPoint;        // for tessellation
};

struct PipelineReflection
{
    ShaderStageFlag mShaderStages;
    std::array<std::unique_ptr<ShaderReflection>, SHADER_STAGE_FLAG_COUNT> mShaderReflections{};
    std::pmr::vector<ShaderResource> mShaderResources;
    std::pmr::vector<ShaderVariable> mShaderVariables;
};

}  // namespace axe::rhi