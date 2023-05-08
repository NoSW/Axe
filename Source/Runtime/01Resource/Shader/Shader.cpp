#include "01Resource/Shader/Shader.hpp"
#include "00Core/Log/Log.hpp"

#include <unordered_map>
#include <span>

namespace axe::resource
{

#include <ShaderHeader.inl>

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
    auto iter = gs_SHADER_BYTE_MAP.find(filename);
    if (iter->second.dxil.pData == nullptr || iter->second.dxil.byteCount == 0)
    {
        AXE_ERROR("Failed to load shader byte(type=dxil) code (filename={})", filename.data());
    }
    return std::span<u8>{iter->second.dxil.pData, iter->second.dxil.byteCount};
}

}  // namespace axe::resource