#pragma once
#include <02Rhi/Config.hpp>
#include <string_view>
#include <unordered_map>

namespace axe::rhi
{
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

enum DescriptorType
{
    DESCRIPTOR_TYPE_UNDEFINED                          = 0,
    DESCRIPTOR_TYPE_SAMPLER                            = 0x01,

    // SRV Read only texture
    DESCRIPTOR_TYPE_TEXTURE                            = (DESCRIPTOR_TYPE_SAMPLER << 1),

    /// UAV Texture
    DESCRIPTOR_TYPE_RW_TEXTURE                         = (DESCRIPTOR_TYPE_TEXTURE << 1),

    // SRV Read only buffer
    DESCRIPTOR_TYPE_BUFFER                             = (DESCRIPTOR_TYPE_RW_TEXTURE << 1),
    DESCRIPTOR_TYPE_BUFFER_RAW                         = (DESCRIPTOR_TYPE_BUFFER | (DESCRIPTOR_TYPE_BUFFER << 1)),

    /// UAV Buffer
    DESCRIPTOR_TYPE_RW_BUFFER                          = (DESCRIPTOR_TYPE_BUFFER << 2),
    DESCRIPTOR_TYPE_RW_BUFFER_RAW                      = (DESCRIPTOR_TYPE_RW_BUFFER | (DESCRIPTOR_TYPE_RW_BUFFER << 1)),

    /// Uniform buffer
    DESCRIPTOR_TYPE_UNIFORM_BUFFER                     = (DESCRIPTOR_TYPE_RW_BUFFER << 2),

    /// Push constant / Root constant
    DESCRIPTOR_TYPE_ROOT_CONSTANT                      = (DESCRIPTOR_TYPE_UNIFORM_BUFFER << 1),

    /// IA
    DESCRIPTOR_TYPE_VERTEX_BUFFER                      = (DESCRIPTOR_TYPE_ROOT_CONSTANT << 1),
    DESCRIPTOR_TYPE_INDEX_BUFFER                       = (DESCRIPTOR_TYPE_VERTEX_BUFFER << 1),
    DESCRIPTOR_TYPE_INDIRECT_BUFFER                    = (DESCRIPTOR_TYPE_INDEX_BUFFER << 1),

    /// Cubemap SRV
    DESCRIPTOR_TYPE_TEXTURE_CUBE                       = (DESCRIPTOR_TYPE_TEXTURE | (DESCRIPTOR_TYPE_INDIRECT_BUFFER << 1)),

    /// RTV / DSV per mip slice
    DESCRIPTOR_TYPE_RENDER_TARGET_MIP_SLICES           = (DESCRIPTOR_TYPE_INDIRECT_BUFFER << 2),

    /// RTV / DSV per array slice
    DESCRIPTOR_TYPE_RENDER_TARGET_ARRAY_SLICES         = (DESCRIPTOR_TYPE_RENDER_TARGET_MIP_SLICES << 1),

    /// RTV / DSV per depth slice
    DESCRIPTOR_TYPE_RENDER_TARGET_DEPTH_SLICES         = (DESCRIPTOR_TYPE_RENDER_TARGET_ARRAY_SLICES << 1),
    DESCRIPTOR_TYPE_RAY_TRACING                        = (DESCRIPTOR_TYPE_RENDER_TARGET_DEPTH_SLICES << 1),
    DESCRIPTOR_TYPE_INDIRECT_COMMAND_BUFFER            = (DESCRIPTOR_TYPE_RAY_TRACING << 1),

    /// Subpass input (only available in Vulkan)
    DESCRIPTOR_TYPE_INPUT_ATTACHMENT                   = (DESCRIPTOR_TYPE_INDIRECT_COMMAND_BUFFER << 1),
    DESCRIPTOR_TYPE_TEXEL_BUFFER                       = (DESCRIPTOR_TYPE_INPUT_ATTACHMENT << 1),
    DESCRIPTOR_TYPE_RW_TEXEL_BUFFER                    = (DESCRIPTOR_TYPE_TEXEL_BUFFER << 1),
    DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER             = (DESCRIPTOR_TYPE_RW_TEXEL_BUFFER << 1),

    /// Khronos extension ray tracing (only available in Vulkan))
    DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE             = (DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER << 1),
    DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_BUILD_INPUT = (DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE << 1),
    DESCRIPTOR_TYPE_SHADER_DEVICE_ADDRESS              = (DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_BUILD_INPUT << 1),
    DESCRIPTOR_TYPE_SHADER_BINDING_TABLE               = (DESCRIPTOR_TYPE_SHADER_DEVICE_ADDRESS << 1),
};

enum ShaderStage
{
    SHADER_STAGE_NONE = 0,
    SHADER_STAGE_VERT,
    SHADER_STAGE_TESC,
    SHADER_STAGE_TESE,
    SHADER_STAGE_GEOM,
    SHADER_STAGE_FRAG,
    SHADER_STAGE_COMP,
    SHADER_STAGE_RAYTRACING,
    SHADER_STAGE_COUNT  = 7,

    SHADER_STAGE_HULL   = SHADER_STAGE_TESC,
    SHADER_STAGE_DOMAIN = SHADER_STAGE_TESE
};

inline constexpr ShaderStage get_shader_stage(std::string_view ext)
{
    if (ext == ".vert") { return SHADER_STAGE_VERT; }
    else if (ext == ".tesc") { return SHADER_STAGE_TESC; }
    else if (ext == ".tese") { return SHADER_STAGE_TESE; }
    else if (ext == ".geom") { return SHADER_STAGE_GEOM; }
    else if (ext == ".frag") { return SHADER_STAGE_FRAG; }
    else if (ext == ".comp") { return SHADER_STAGE_COMP; }
    else if (ext == ".rgen" || ext == ".rmiss" || ext == ".rchit" ||
             ext == ".rint" || ext == ".rahit" || ext == "rcall") { return SHADER_STAGE_RAYTRACING; }
    else { return SHADER_STAGE_NONE; }
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
    ShaderStage mShaderStage;
    TextureDimension mDim;
    DescriptorType mType;
    u32 mSet;
    u32 mLBindingLocation;
    u32 mSize;
};

struct ShaderVariable
{
    std::string_view mName;
    u32 mParentIndex;
    u32 mOffset;
    u32 mSize;
};

struct ShaderReflection
{
    std::pmr::vector<VertexInput> mVertexInputs;
    std::pmr::vector<ShaderResource> mShaderResources;
    std::pmr::vector<ShaderVariable> mShaderVariables;
    std::string_view mEntryPoint;
    ShaderStage mShaderStage;
    u32 mNumThreadsPerGroup[3];  // for compute shader
    u32 mNumControlPoint[3];     // for tessellation
};

struct PipelineReflection
{
    ShaderStage mShaderStages;
    std::pmr::vector<ShaderReflection> mShaderReflections;
    std::pmr::vector<ShaderResource> mShaderResources;
    std::pmr::vector<ShaderVariable> mShaderVariables;
    u32 mVertexStageIndex;
    u32 mHullStageIndex;
    u32 mDomainStageIndex;
    u32 mGeometryStageIndex;
    u32 mPixelStageIndex;
};

}  // namespace axe::rhi