#include <00Core/IO/IO.hpp>

#include "00Core/Log/Log.hpp"

#include <fstream>

namespace axe::io
{
// Returns true only when a non-empty file is successfully read.
bool read_file_binary(const std::filesystem::path& filePath, std::pmr::vector<u8>& out) noexcept
{
    if (std::filesystem::exists(filePath) && std::filesystem::is_regular_file(filePath))
    {
        std::ifstream ifs(filePath, std::ios::in | std::ios::binary);
        if (!ifs.fail())
        {
            out.assign(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
            ifs.close();
            return !out.empty();
        }
        else
        {
            AXE_ERROR("Failed to read existing {} due to unknown error", filePath.string());
            return false;
        }
    }
    else
    {
        AXE_ERROR("Failed to read {} that does NOT exist", filePath.string());
        return false;
    }
}

bool read_file_binary_ext(const std::filesystem::path& filePath, const std::string_view extension, std::pmr::vector<u8>& out) noexcept
{
    if (extension == filePath.extension().string())
    {
        return read_file_binary(filePath, out);
    }
    else
    {
        AXE_ERROR("Failed to read {} due to mismatch file extension", filePath.string());
        return false;
    }
}

bool read_file_binary_ext(const std::filesystem::path& filePath, const std::vector<std::string_view> extensions, std::pmr::vector<u8>& out) noexcept
{
    std::string fileExt = filePath.extension().string();
    bool foundExt       = false;
    for (const auto& ext : extensions)
    {
        if (ext == fileExt)
        {
            foundExt = true;
            break;
        }
    }

    if (foundExt)
    {
        return read_file_binary(filePath, out);
    }
    else
    {
        AXE_ERROR("Failed to read {} due to mismatch file extension", filePath.string());
        return false;
    }
}

}  // namespace axe::io