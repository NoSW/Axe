#include "01Resource/Shader/Shader.hpp"
#include "00Core/Log/Log.hpp"
#include <unordered_map>

namespace axe::resource
{

#include <ShaderHeader.inl>

const std::span<u8> get_spv_byte_code(std::string_view filename) noexcept
{
    auto iter = gs_SHADER_BYTE_MAP.find(filename);
    if (iter->second.mSpv.mpData == nullptr || iter->second.mSpv.mByteCount == 0)
    {
        AXE_ERROR("Failed to load shader byte(type=spv) code (filename={})", filename.data());
    }
    return std::span<u8>{iter->second.mSpv.mpData, iter->second.mSpv.mByteCount};
}

const std::span<u8> get_dxil_byte_code(std::string_view filename) noexcept
{
    auto iter = gs_SHADER_BYTE_MAP.find(filename);
    if (iter->second.mDxil.mpData == nullptr || iter->second.mDxil.mByteCount == 0)
    {
        AXE_ERROR("Failed to load shader byte(type=dxil) code (filename={})", filename.data());
    }
    return std::span<u8>{iter->second.mDxil.mpData, iter->second.mDxil.mByteCount};
}

}  // namespace axe::resource