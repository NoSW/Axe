#pragma once
#include "00Core/Config.hpp"
#include <vector>
#include <string_view>
#include <filesystem>

namespace axe::io
{

bool read_file_binary(const std::filesystem::path& filePath, std::pmr::vector<u8>& out) noexcept;

bool read_file_binary_ext(const std::filesystem::path& filePath, const std::string_view extension, std::pmr::vector<u8>& out) noexcept;

bool read_file_binary_ext(const std::filesystem::path& filePath, const std::vector<std::string_view> extensions, std::pmr::vector<u8>& out) noexcept;

}  // namespace axe::io