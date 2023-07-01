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

enum class WaveOpsSupportFlag
{
    NONE                 = 0,
    BASIC_BIT            = 1 << 0,
    VOTE_BIT             = 1 << 1,
    ARITHMETIC_BIT       = 1 << 2,
    BALLOT_BIT           = 1 << 3,
    SHUFFLE_BIT          = 1 << 4,
    SHUFFLE_RELATIVE_BIT = 1 << 5,
    CLUSTERED_BIT        = 1 << 6,
    QUAD_BIT             = 1 << 7,
    PARTITIONED_BIT_NV   = 1 << 8,
    ALL                  = 0x7FFFFFFF
};
using WaveOpsSupportFlagOneBit = WaveOpsSupportFlag;

enum class GpuVenderId
{
    NVIDIA  = 0x10DE,
    AMD     = 0x1002,
    AMD_1   = 0x1022,
    INTEL   = 0x163C,
    INTEL_1 = 0x8086,
    INTEL_2 = 0x8087
};

// default capability levels of the backend
enum CapabilityLevel
{
    MAX_INSTANCE_EXTENSIONS       = 64,
    MAX_DEVICE_EXTENSIONS         = 64,
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

enum class FenceStatus
{
    COMPLETE = 0,
    INCOMPLETE,
    NOTSUBMITTED,
};

enum class QueueTypeFlag
{
    GRAPHICS = 0,
    COMPUTE  = 1 << 0,
    TRANSFER = 1 << 1,
    COUNT,
    UNDEFINED = COUNT
};
using QueueTypeOneBit = QueueTypeFlag;

enum class QueueFlag : u32
{
    NONE                = 0x0,
    DISABLE_GPU_TIMEOUT = 0x1,
    INIT_MICROPROFILE   = 0x2,
    UNDEFINED           = 0xFFFFFFFF
};
using QueueFlagOneBit = QueueFlag;

enum class QueuePriority
{
    NORMAL,
    HIGH,
    GLOBAL_REALTIME,
    UNDEFINED
};

enum class FilterType
{
    NEAREST = 0,
    LINEAR  = 1,
};

enum class MipMapMode
{
    NEAREST = 0,
    LINEAR  = 1,
};

enum class AddressMode
{
    MIRROR          = 0,
    REPEAT          = 1,
    CLAMP_TO_EDGE   = 2,
    CLAMP_TO_BORDER = 3,
};

enum class BlendConstant
{
    ZERO = 0,
    ONE,
    SRC_COLOR,
    ONE_MINUS_SRC_COLOR,
    DST_COLOR,
    ONE_MINUS_DST_COLOR,
    SRC_ALPHA,
    ONE_MINUS_SRC_ALPHA,
    DST_ALPHA,
    ONE_MINUS_DST_ALPHA,
    SRC_ALPHA_SATURATE,
    BLEND_FACTOR,
    ONE_MINUS_BLEND_FACTOR,
    UNDEFINED
};

enum class BlendMode
{
    ADD,
    SUBTRACT,
    REVERSE_SUBTRACT,
    MIN,
    MAX,
    COUNT,
    UNDEFINED = COUNT,
};

enum class BlendStateTargetsFlag
{
    TARGET_0   = 1 << 0,
    TARGET_1   = 1 << 1,
    TARGET_2   = 1 << 2,
    TARGET_3   = 1 << 3,
    TARGET_4   = 1 << 4,
    TARGET_5   = 1 << 5,
    TARGET_6   = 1 << 6,
    TARGET_7   = 1 << 7,
    TARGET_ALL = 0xFF,
};
using BlendStateTargetsFlagOneBit = BlendStateTargetsFlag;

enum class StencilOp
{
    KEEP,
    SET_ZERO,
    REPLACE,
    INVERT,
    INCR,
    DECR,
    INCR_SAT,
    DECR_SAT,
    COUNT,
};

enum class Channel
{
    NONE  = 0,
    RED   = 1 << 0,
    GREEN = 1 << 1,
    BLUE  = 1 << 2,
    ALPHA = 1 << 3,
    ALL   = RED | GREEN | BLUE | ALPHA,
};

enum class CompareMode
{
    NEVER    = 0,
    LESS     = 1,
    EQUAL    = 2,
    LEQUAL   = 3,
    GREATER  = 4,
    NOTEQUAL = 5,
    GEQUAL   = 6,
    ALWAYS   = 7,
    COUNT    = 8
};

enum class SamplerRange
{
    FULL   = 0,
    NARROW = 1,
};

enum class SampleLocation
{
    COSITED  = 0,
    MIDPOINT = 1,
};

enum class SamplerModelConversion
{
    RGB_IDENTITY   = 0,
    YCBCR_IDENTITY = 1,
    YCBCR_709      = 2,
    YCBCR_601      = 3,
    YCBCR_2020     = 4,
};

enum class ResourceStateFlags
{
    UNDEFINED                         = 0,
    VERTEX_AND_CONSTANT_BUFFER        = 1 << 0,
    INDEX_BUFFER                      = 1 << 1,
    RENDER_TARGET                     = 1 << 2,
    UNORDERED_ACCESS                  = 1 << 3,
    DEPTH_WRITE                       = 1 << 4,
    DEPTH_READ                        = 1 << 5,
    NON_PIXEL_SHADER_RESOURCE         = 1 << 6,
    PIXEL_SHADER_RESOURCE             = 1 << 7,
    STREAM_OUT                        = 1 << 8,
    INDIRECT_ARGUMENT                 = 1 << 9,
    COPY_DEST                         = 1 << 10,
    COPY_SOURCE                       = 1 << 11,
    PRESENT                           = 1 << 12,
    COMMON                            = 1 << 13,
    RAYTRACING_ACCELERATION_STRUCTURE = 1 << 14,
    SHADING_RATE_SOURCE               = 1 << 15,
    SHADER_RESOURCE                   = NON_PIXEL_SHADER_RESOURCE | PIXEL_SHADER_RESOURCE,
    GENERICC_READ                     = VERTEX_AND_CONSTANT_BUFFER | INDEX_BUFFER | RENDER_TARGET |
                    PIXEL_SHADER_RESOURCE | INDIRECT_ARGUMENT | COPY_SOURCE,
};
using ResourceStateFlagsOneBit = ResourceStateFlags;

enum class TextureCreationFlags
{
    // Default flag (Texture will use default allocation strategy decided by the api specific allocator)
    NONE                  = 0,
    // Texture will allocate its own memory (COMMITTED resource)
    OWN_MEMORY_BIT        = 1 << 0,
    // Texture will be allocated in memory which can be shared among multiple processes
    EXPORT_BIT            = 1 << 1,
    // Texture will be allocated in memory which can be shared among multiple gpus
    EXPORT_ADAPTER_BIT    = 1 << 2,
    // Texture will be imported from a handle created in another process
    IMPORT_BIT            = 1 << 3,
    // Use ESRAM to store this texture
    ESRAM                 = 1 << 4,
    // Use on-tile memory to store this texture
    ON_TILE               = 1 << 5,
    // Prevent compression meta data from generating (XBox)
    NO_COMPRESSION        = 1 << 6,
    // Force 2D instead of automatically determining dimension based on width, height, depth
    FORCE_2D              = 1 << 7,
    // Force 3D instead of automatically determining dimension based on width, height, depth
    FORCE_3D              = 1 << 8,
    // Display target
    ALLOW_DISPLAY_TARGET  = 1 << 9,
    // Create an sRGB texture.
    SRGB                  = 1 << 10,
    // Create a normal map texture
    NORMAL_MAP            = 1 << 11,
    // Fast clear
    FAST_CLEAR            = 1 << 12,
    // Fragment mask
    FRAG_MASK             = 1 << 13,
    // Doubles the amount of array layers of the texture when rendering VR. Also forces the texture to be a 2D Array texture.
    VR_MULTIVIEW          = 1 << 14,
    // Binds the FFR fragment density if this texture is used as a render target.
    VR_FOVEATED_RENDERING = 1 << 15,
};
using TextureCreationFlagsOneBit = TextureCreationFlags;

enum class MSAASampleCount
{
    COUNT_1     = 1,
    COUNT_2     = 2,
    COUNT_4     = 4,
    COUNT_8     = 8,
    COUNT_16    = 16,
    COUNT_COUNT = 5,
};

enum class ResourceFlag
{
    UNDEFINED                         = 0,
    VERTEX_AND_CONSTANT_BUFFER        = 1 << 0,
    INDEX_BUFFER                      = 1 << 1,
    RENDER_TARGET                     = 1 << 2,
    UNORDERED_ACCESS                  = 1 << 3,
    DEPTH_WRITE                       = 1 << 4,
    DEPTH_READ                        = 1 << 5,
    NON_PIXEL_SHADER_RESOURCE         = 1 << 6,
    PIXEL_SHADER_RESOURCE             = 1 << 7,
    STREAM_OUT                        = 1 << 8,
    INDIRECT_ARGUMENT                 = 1 << 9,
    COPY_DEST                         = 1 << 10,
    COPY_SOURCE                       = 1 << 11,
    PRESENT                           = 1 << 12,
    COMMON                            = 1 << 13,
    RAYTRACING_ACCELERATION_STRUCTURE = 1 << 14,
    SHADING_RATE_SOURCE               = 1 << 15,
    SHADER_RESOURCE                   = NON_PIXEL_SHADER_RESOURCE | PIXEL_SHADER_RESOURCE,
    GENERICC_READ                     = VERTEX_AND_CONSTANT_BUFFER | INDEX_BUFFER |
                    NON_PIXEL_SHADER_RESOURCE | PIXEL_SHADER_RESOURCE |
                    INDIRECT_ARGUMENT | COPY_SOURCE,
};
using ResourceFlagOneBit = ResourceFlag;

enum class ResourceMemoryUsage
{

    UNKNOWN    = 0,  // No intended memory usage specified.
    GPU_ONLY   = 1,  // Memory will be used on device only, no need to be mapped on host.
    CPU_ONLY   = 2,  // Memory will be mapped on host. Could be used for transfer to device.
    CPU_TO_GPU = 3,  // Memory will be used for frequent (dynamic) updates from host and reads on device.
    GPU_TO_CPU = 4,  // Memory will be used for writing on device and readback on host.
    COUNT,
    MAX_ENUM = 0x7FFFFFFF
};

enum class BufferCreationFlags
{
    NONE                        = 1 << 0,  // Default flag (Buffer will use aliased memory, buffer will not be cpu accessible until mapBuffer is called)
    OWN_MEMORY_BIT              = 1 << 2,  // Buffer will allocate its own memory (COMMITTED resource)
    PERSISTENT_MAP_BIT          = 1 << 3,  // Buffer will be persistently mapped
    ESRAM                       = 1 << 4,  // Use ESRAM to store this buffer
    NO_DESCRIPTOR_VIEW_CREATION = 1 << 5,  // Flag to specify not to allocate descriptors for the resource
    HOST_VISIBLE_VKONLY         = 1 << 6,  // Memory Host Flags
    HOST_COHERENT_VKONLY        = 1 << 7,  // Memory Host Flags

};
using BufferCreationFlagsOneBit = BufferCreationFlags;

enum class TextureDimension
{
    DIM_1D,
    DIM_2D,
    DIM_2DMS,
    DIM_3D,
    DIM_CUBE,
    DIM_1D_ARRAY,
    DIM_2D_ARRAY,
    DIM_2DMS_ARRAY,
    DIM_CUBE_ARRAY,
    DIM_COUNT,
    DIM_UNDEFINED,

};

enum class IndirectArgumentType
{
    INVALID,
    DRAW,
    DRAW_INDEX,
    DISPATCH,
    VERTEX_BUFFER,
    INDEX_BUFFER,
    CONSTANT,
    CONSTANT_BUFFER_VIEW_DXONLY,   // only for dx
    SHADER_RESOURCE_VIEW_DXONLY,   // only for dx
    UNORDERED_ACCESS_VIEW_DXONLY,  // only for dx
};

enum class DescriptorTypeFlag
{
    UNDEFINED                                 = 0,
    SAMPLER                                   = 1 << 0,
    TEXTURE                                   = 1 << 1,                // SRV Read only texture
    RW_TEXTURE                                = 1 << 2,                /// UAV Texture
    BUFFER                                    = 1 << 3,                // SRV Read only buffer
    BUFFER_RAW                                = (1 << 4) | BUFFER,     // SRV Read only buffer
    RW_BUFFER                                 = 1 << 5,                /// UAV Buffer
    RW_BUFFER_RAW                             = (1 << 6) | RW_BUFFER,  /// UAV Buffer
    UNIFORM_BUFFER                            = 1 << 7,                /// Uniform buffer
    ROOT_CONSTANT                             = 1 << 8,                /// Push constant / Root constant

    /// IA
    VERTEX_BUFFER                             = 1 << 9,
    INDEX_BUFFER                              = 1 << 10,
    INDIRECT_BUFFER                           = 1 << 11,

    TEXTURE_CUBE                              = 1 << 12 | TEXTURE,  /// Cubemap SRV
    RENDER_TARGET_MIP_SLICES                  = 1 << 13,            /// RTV / DSV per mip slice
    RENDER_TARGET_ARRAY_SLICES                = 1 << 14,            /// RTV / DSV per array slice

    /// RTV / DSV per depth slice
    RENDER_TARGET_DEPTH_SLICES                = 1 << 15,
    RAY_TRACING                               = 1 << 16,
    INDIRECT_COMMAND_BUFFER                   = 1 << 17,

    /// Subpass input (only available in Vulkan)
    INPUT_ATTACHMENT_VKONLY                   = 1 << 18,
    TEXEL_BUFFER_VKONLY                       = 1 << 19,
    RW_TEXEL_BUFFER_VKONLY                    = 1 << 20,
    COMBINED_IMAGE_SAMPLER_VKONLY             = 1 << 21,

    /// Khronos extension ray tracing (only available in Vulkan))
    ACCELERATION_STRUCTURE_VKONLY             = 1 << 22,
    ACCELERATION_STRUCTURE_BUILD_INPUT_VKONLY = 1 << 23,
    SHADER_DEVICE_ADDRESS_VKONLY              = 1 << 24,
    SHADER_BINDING_TABLE_VKONLY               = 1 << 25,
};
using DescriptorTypeFlagOneBit = DescriptorTypeFlag;

enum class ShaderStageFlag
{
    NONE       = 0,
    VERT       = 1 << 0,
    TESC       = 1 << 1,
    TESE       = 1 << 2,
    GEOM       = 1 << 3,
    FRAG       = 1 << 4,
    COMP       = 1 << 5,
    RAYTRACING = 1 << 6,
    MAX        = RAYTRACING + 1,
    COUNT      = 7,

    HULL       = TESC,
    DOMAINN    = TESE,

    GRAPHICS   = VERT | TESC | TESE | GEOM | FRAG,
};
using ShaderStageFlagOneBit = ShaderStageFlag;  // must be the single bit

enum class ShaderModel
{
    // (From) 5.1 is supported on all DX12 hardware
    SM_5_1 = 0x51,
    SM_6_0 = 0x60,
    SM_6_1 = 0x61,
    SM_6_2 = 0x62,
    SM_6_3 = 0x63,
    SM_6_4 = 0x64,
    SM_6_5 = 0x65,
    SM_6_6 = 0x66,
    SM_6_7 = 0x67,
    SM_HIGHEST
};

enum class ShaderStageLoadFlag
{
    NONE                  = 0,
    ENABLE_PS_PRIMITIVEID = 1 << 0,
};
using ShaderStageLoadFlagOneBit = ShaderStageLoadFlag;

enum class PipelineType
{
    UNDEFINED = 0,
    COMPUTE,
    GRAPHICS,
    RAYTRACING,
    COUNT,
};

enum class DescriptorUpdateFrequency : u8
{
    NONE = 0,
    PER_FRAME,
    PER_BATCH,
    PER_DRAW,
    COUNT,
};

enum class RootSignatureFlags
{
    NONE      = 0,       // Default flag
    LOCAL_BIT = 1 << 0,  // used mainly in raytracing shaders
};
using RootSignatureFlagsOneBit = RootSignatureFlags;

enum class VertexAttribRate
{
    VERTEX   = 0,
    INSTANCE = 1,
    COUNT,
};

enum class ShaderSemantic
{
    UNDEFINED = 0,
    POSITION,
    NORMAL,
    COLOR,
    TANGENT,
    BITANGENT,
    JOINTS,
    WEIGHTS,
    SHADING_RATE,
    TEXCOORD0,
    TEXCOORD1,
    TEXCOORD2,
    TEXCOORD3,
    TEXCOORD4,
    TEXCOORD5,
    TEXCOORD6,
    TEXCOORD7,
    TEXCOORD8,
    TEXCOORD9,
};

enum class CullMode
{
    NONE = 0,
    BACK,
    FRONT,
    BOTH,
    COUNT,
};

enum class FrontFace
{
    CCW = 0,
    CW
};

enum class FillMode
{
    SOLID = 0,
    WIREFRAME,
    COUNT,
};

enum class LoadActionType
{
    DONT_CARE,
    LOAD,
    CLEAR,
    COUNT
};

enum class StoreActionType
{
    STORE = 0,
    DONT_CARE,
    NONE,
    COUNT
};

enum class PrimitiveTopology
{
    POINT_LIST = 0,
    LINE_LIST,
    LINE_STRIP,
    TRI_LIST,
    TRI_STRIP,
    PATCH_LIST,
    COUNT,
};

enum class AdapterType
{
    _OTHER          = 0,
    _INTEGRATED_GPU = 1,
    _DISCRETE_GPU   = 2,
    _VIRTUAL_GPU    = 3,
    _CPU            = 4,
    _COUNT
};

inline constexpr u32 GRAPHICS_API_COUNT = 2;
enum class GraphicsApiFlag
{
    UNDEFINED = AXE_02RHI_API_FLAG_NULL,
    VULKAN    = AXE_02RHI_API_FLAG_VULKAN,
    D3D12     = AXE_02RHI_API_FLAG_D3D12,
    AVAILABLE = AXE_02RHI_API_FLAG_AVAILABLE,
};
using GraphicsApiFlagOneBit = GraphicsApiFlag;

struct VertexInput
{
    std::string_view name;  // resource name
    u32 size;               // The size of the attribute
};

struct ShaderResource  // all other resources except for ShaderVariable (vertex in/out, ...)
{
    std::string_view name;
    ShaderStageFlag usedShaderStage;
    TextureDimension dim;
    DescriptorTypeFlag type;
    u32 mSet;
    u32 bindingLocation;
    u32 size;  // element count of array

    bool operator==(const ShaderResource& b) const { return type == b.type && mSet == b.mSet && bindingLocation == b.bindingLocation && name == b.name; }

    bool operator!=(const ShaderResource& b) const { return !(*this == b); }
};

struct ShaderVariable  // resources updated frequently (uniform buffer, root constant(i.e., pushConstants in VK))
{
    std::string_view name;
    u32 parentIndex;
    u32 offset;
    u32 size;
    bool operator==(const ShaderVariable& b) const { return offset == b.offset && size == b.size && name == b.name; }

    bool operator!=(const ShaderVariable& b) const { return !(*this == b); }
};

struct ShaderReflection
{
    std::pmr::vector<VertexInput> vertexInputs;
    std::pmr::vector<ShaderResource> shaderResources;
    std::pmr::vector<ShaderVariable> shaderVariables;
    std::pmr::string entryPoint_VkOnly;
    ShaderStageFlagOneBit shaderStage;
    u32 numThreadsPerGroup[3];  // for compute shader
    u32 numControlPoint;        // for tessellation
};

struct PipelineReflection
{
    ShaderStageFlag shaderStages;
    std::array<ShaderReflection, (u32)ShaderStageFlag::COUNT> shaderReflections{};
    std::pmr::vector<ShaderResource> shaderResources;  //  unique shader resources in all stages, (we need indexing here, so use vector just fine)
    std::pmr::vector<ShaderVariable> shaderVariables;  // unique shader variables  in all stages
};

}  // namespace axe::rhi