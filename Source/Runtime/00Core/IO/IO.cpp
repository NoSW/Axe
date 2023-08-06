#include <00Core/IO/IO.hpp>

#include "00Core/Log/Log.hpp"

#include <fstream>

// @DISCUSSION@ when/where/who to report error message?
//  1. exception handling (pros and cons explained by https://google.github.io/styleguide/cppguide.html#Exceptions)
//    - separate error handling from normal code path :-)
//    - harmful to performance. history reason: most c++ libraries do not prepare for exception
//    - How to disable exception?
//        - clang/gcc: add compile flag -fno-exceptions (will call abort() when exception is thrown)
//        - MSVC:https://learn.microsoft.com/en-us/cpp/build/reference/eh-exception-handling-model?view=msvc-170
//    - NOTE: If an exception escapes from a function marked noexcept, the program crashes via std::terminate.
//  2. error code
//     - good for performance, standard C-Style
//     - easy to be ignored most of the time, :-(may set [[nodiscard]] to force caller to check the return value)
//  3. logging at anytime
//      - very easy-to-use, and debug friendly since we can detect error at first time :-)
//      - not good for performance, and suitable for quick prototype bot for production-ready code (Adopted by this project)
//  4. error handling object. e.g. std::optional, std::expected;
//       - little complex since we need to handle these objects
//  5. assert
//     - a good tool, but dev-time only
//  6. error callbacks
//     - too complex, make the api bloated

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
            out.resize(ifs.seekg(0, std::ios::end).tellg());
            ifs.seekg(0, std::ios::beg).read((char*)(out.data()), static_cast<std::streamsize>(out.size()));
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