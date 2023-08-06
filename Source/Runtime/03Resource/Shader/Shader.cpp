#include "03Resource/Shader/Shader.hpp"
#include "00Core/Log/Log.hpp"
#include "00Core/Container/String.hpp"

#include <unordered_map>
#include <span>

namespace axe::resource
{

#include <ShaderHeader.generated.inl>  //  containg all spir-v code generated from Shader/**/*.glsl

const std::span<u8> get_spv_byte_code(std::string_view filename) noexcept
{
    auto iter = gs_SHADER_BYTE_MAP.find(filename);
    if (iter->second.spv.pData == nullptr || iter->second.spv.byteCount == 0)
    {
        AXE_ERROR("Failed to load shader byte(type=spv) code (filename={})", filename.data());
    }
    return std::span<u8>{iter->second.spv.pData, iter->second.spv.byteCount};
}

const std::span<u8> get_dxil_byte_code(std::string_view filename) noexcept
{
#if _WIN32
    auto iter = gs_SHADER_BYTE_MAP.find(filename);
    if (iter->second.dxil.pData == nullptr || iter->second.dxil.byteCount == 0)
    {
#if AXE_(false, "D3D12 is not implemented yet")
        AXE_ERROR("Failed to load shader byte(type=dxil) code (filename={})", filename.data());
#endif
    }
    return std::span<u8>{iter->second.dxil.pData, iter->second.dxil.byteCount};
#else
    return std::span<u8, 0>{};
#endif
}

static constexpr rhi::ShaderStageFlag _ext_to_shader_stage(std::string_view ext)
{
    if (ext == ".vert") { return rhi::ShaderStageFlag::VERT; }
    else if (ext == ".tesc") { return rhi::ShaderStageFlag::TESC; }
    else if (ext == ".tese") { return rhi::ShaderStageFlag::TESE; }
    else if (ext == ".geom") { return rhi::ShaderStageFlag::GEOM; }
    else if (ext == ".frag") { return rhi::ShaderStageFlag::FRAG; }
    else if (ext == ".comp") { return rhi::ShaderStageFlag::COMP; }
    else if (ext == ".rgen" || ext == ".rmiss" || ext == ".rchit" ||
             ext == ".rint" || ext == ".rahit" || ext == "rcall") { return rhi::ShaderStageFlag::RAYTRACING; }
    else { return rhi::ShaderStageFlag::NONE; }
};

rhi::ShaderStageDesc get_shader_stage(std::string_view filename) noexcept
{
    std::array<std::span<u8>, rhi::GRAPHICS_API_COUNT> byteCodes;
    byteCodes[bit2id(rhi::GraphicsApiFlag::VULKAN)] = get_spv_byte_code(filename);
    byteCodes[bit2id(rhi::GraphicsApiFlag::D3D12)]  = get_dxil_byte_code(filename);

    return rhi::ShaderStageDesc{
        .name        = filename,
        .mStage      = _ext_to_shader_stage(str::last_ext(str::rm_ext(filename))),
        .byteCode    = byteCodes,
        .mEntryPoint = "main",
        .flags       = rhi::ShaderStageLoadFlag::NONE};
}

}  // namespace axe::resource